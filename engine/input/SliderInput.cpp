#include "SliderInput.h"
#include "ImGuiManager.h"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace {
// 受信スレッドの待ち時間（ms）。終了要求にすぐ気づけるよう短めに区切って待つ
constexpr DWORD kReadTimeoutMs = 100;
// 自動検出で1つのポートを試す時間（ms）。ボードによっては開いた瞬間にリセットされるので長めに待つ
constexpr DWORD kProbeTimeoutMs = 2500;
// この時間（ms）値が届かなければ「未接続」とみなす
constexpr ULONGLONG kConnectionTimeoutMs = 1000;
// 見つからなかった時に、次に探し直すまでの間隔（ms）
constexpr DWORD kRetryIntervalMs = 1000;

// PC に存在する COM ポート名の一覧をレジストリから取得する（例: L"COM3"）
std::vector<std::wstring> EnumerateComPorts() {
	std::vector<std::wstring> ports;
	HKEY key = nullptr;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &key) != ERROR_SUCCESS) {
		return ports;
	}
	for (DWORD index = 0;; ++index) {
		wchar_t name[256];
		DWORD nameLength = static_cast<DWORD>(std::size(name));
		wchar_t data[64];
		DWORD dataSize = sizeof(data);
		DWORD type = 0;
		const LONG result = RegEnumValueW(key, index, name, &nameLength, nullptr, &type, reinterpret_cast<BYTE*>(data), &dataSize);
		if (result == ERROR_NO_MORE_ITEMS) {
			break;
		}
		if (result == ERROR_SUCCESS && type == REG_SZ) {
			data[std::size(data) - 1] = L'\0';
			ports.emplace_back(data);
		}
	}
	RegCloseKey(key);
	return ports;
}

std::string ToNarrow(const std::wstring& text) {
	std::string result;
	for (wchar_t c : text) {
		result += static_cast<char>(c);
	}
	return result;
}
} // namespace

TuboEngine::SliderInput* TuboEngine::SliderInput::instance = nullptr;

TuboEngine::SliderInput* TuboEngine::SliderInput::GetInstance() {
	if (instance == nullptr) {
		instance = new SliderInput();
	}
	return instance;
}

void TuboEngine::SliderInput::Initialize(const std::wstring& portName, DWORD baudRate) {
	if (running_) {
		return;
	}
	baudRate_ = baudRate;
	{
		std::lock_guard<std::mutex> lock(portMutex_);
		requestedPort_ = portName;
	}
	running_ = true;
	thread_ = std::thread([this] { ThreadMain(); });
}

void TuboEngine::SliderInput::Finalize() {
	running_ = false;
	if (thread_.joinable()) {
		thread_.join();
	}
	ClosePort();
	delete instance;
	instance = nullptr;
}

void TuboEngine::SliderInput::Update() {
	// 受信頻度（デバッグ表示用）
	const ULONGLONG now = GetTickCount64();
	if (now - lastSecondTick_ >= 1000) {
		const uint32_t lines = receivedLines_.load();
		linesPerSecond_ = lines - lineCountAtLastSecond_;
		lineCountAtLastSecond_ = lines;
		lastSecondTick_ = now;
	}

	if (receivedLines_.load() == 0) {
		return; // まだ一度も値が届いていない
	}

	const int range = (std::max)(rawMax_ - rawMin_, 1);
	for (int i = 0; i < kAxisCount; ++i) {
		raw_[i] = latestRaw_[i].load();

		// 生の値 → 0〜1
		float t = static_cast<float>(raw_[i] - rawMin_) / static_cast<float>(range);
		// 両端の不感帯（端まで動かした時にきっちり 0 / 1 になるように）
		if (edgeDeadZone_ > 0.0f && edgeDeadZone_ < 0.5f) {
			t = (t - edgeDeadZone_) / (1.0f - 2.0f * edgeDeadZone_);
		}
		t = std::clamp(t, 0.0f, 1.0f);

		valuePre_[i] = value_[i];
		// 平滑化（前の値に少しずつ近づける）
		value_[i] = hasValue_ ? value_[i] * smoothing_ + t * (1.0f - smoothing_) : t;
		if (!hasValue_) {
			valuePre_[i] = value_[i];
		}
	}
	hasValue_ = true;
}

float TuboEngine::SliderInput::Get(int axis) const {
	if (axis < 0 || axis >= kAxisCount) return 0.0f;
	return value_[axis];
}

float TuboEngine::SliderInput::GetPrevious(int axis) const {
	if (axis < 0 || axis >= kAxisCount) return 0.0f;
	return valuePre_[axis];
}

int TuboEngine::SliderInput::GetRaw(int axis) const {
	if (axis < 0 || axis >= kAxisCount) return 0;
	return raw_[axis];
}

void TuboEngine::SliderInput::SetSmoothing(float smoothing) {
	smoothing_ = std::clamp(smoothing, 0.0f, 0.95f);
}

void TuboEngine::SliderInput::SetPort(const std::wstring& portName) {
	{
		std::lock_guard<std::mutex> lock(portMutex_);
		requestedPort_ = portName;
	}
	reconnectRequest_ = true;
}

std::wstring TuboEngine::SliderInput::GetConnectedPort() const {
	std::lock_guard<std::mutex> lock(portMutex_);
	return connectedPort_;
}

HANDLE TuboEngine::SliderInput::OpenPort(const std::wstring& portName) const {
	// COM10 以上は "\\.\" を付けないと開けないので常に付ける
	const std::wstring path = L"\\\\.\\" + portName;
	HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (handle == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}

	DCB dcb{};
	dcb.DCBlength = sizeof(DCB);
	if (!GetCommState(handle, &dcb)) {
		CloseHandle(handle);
		return INVALID_HANDLE_VALUE;
	}
	dcb.BaudRate = baudRate_;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.fBinary = TRUE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE; // DTR が立っていないと送ってこないボードがある
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	if (!SetCommState(handle, &dcb)) {
		CloseHandle(handle);
		return INVALID_HANDLE_VALUE;
	}

	// データがあればすぐ返し、無ければ最大 kReadTimeoutMs だけ待つ
	COMMTIMEOUTS timeouts{};
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = kReadTimeoutMs;
	SetCommTimeouts(handle, &timeouts);
	PurgeComm(handle, PURGE_RXCLEAR | PURGE_RXABORT);
	return handle;
}

bool TuboEngine::SliderInput::ReadLines(HANDLE handle, DWORD timeoutMs) {
	const ULONGLONG start = GetTickCount64();
	bool parsed = false;
	do {
		char buffer[256];
		DWORD readSize = 0;
		if (!ReadFile(handle, buffer, sizeof(buffer), &readSize, nullptr)) {
			// ケーブルが抜けた等。呼び出し側でポートを閉じさせる
			readFailed_ = true;
			return false;
		}
		for (DWORD i = 0; i < readSize; ++i) {
			const char c = buffer[i];
			if (c == '\n') {
				parsed = ParseLine(lineBuffer_) || parsed;
				lineBuffer_.clear();
			} else if (c != '\r') {
				lineBuffer_ += c;
				if (lineBuffer_.size() > 256) {
					lineBuffer_.clear(); // 改行が来ないゴミデータ
				}
			}
		}
		if (parsed) {
			return true;
		}
	} while (running_ && GetTickCount64() - start < timeoutMs);
	return parsed;
}

bool TuboEngine::SliderInput::ParseLine(const std::string& line) {
	int v[kAxisCount] = {};
	// "===== started =====" のような数値でない行は読み飛ばす
	if (sscanf_s(line.c_str(), "%d ,%d ,%d ,%d", &v[0], &v[1], &v[2], &v[3]) != kAxisCount) {
		return false;
	}
	for (int i = 0; i < kAxisCount; ++i) {
		latestRaw_[i] = v[i];
	}
	++receivedLines_;
	return true;
}

void TuboEngine::SliderInput::ClosePort() {
	if (handle_ != INVALID_HANDLE_VALUE) {
		CloseHandle(handle_);
		handle_ = INVALID_HANDLE_VALUE;
	}
	connected_ = false;
	lineBuffer_.clear();
	std::lock_guard<std::mutex> lock(portMutex_);
	connectedPort_.clear();
}

void TuboEngine::SliderInput::ThreadMain() {
	ULONGLONG lastReceiveTick = 0;

	while (running_) {
		if (reconnectRequest_.exchange(false)) {
			ClosePort();
		}

		// ---- 未接続：ポートを開く ----
		if (handle_ == INVALID_HANDLE_VALUE) {
			std::wstring requested;
			{
				std::lock_guard<std::mutex> lock(portMutex_);
				requested = requestedPort_;
			}
			// 指定があればそのポートだけ、無ければ PC にある COM ポートを順に試す
			std::vector<std::wstring> candidates = requested.empty() ? EnumerateComPorts() : std::vector<std::wstring>{requested};
			for (const std::wstring& port : candidates) {
				if (!running_ || reconnectRequest_) break;
				HANDLE handle = OpenPort(port);
				if (handle == INVALID_HANDLE_VALUE) continue;
				// スライダーの値が届くポートか確かめる（別の機器のポートは採用しない）
				if (ReadLines(handle, kProbeTimeoutMs)) {
					handle_ = handle;
					lastReceiveTick = GetTickCount64();
					connected_ = true;
					std::lock_guard<std::mutex> lock(portMutex_);
					connectedPort_ = port;
					break;
				}
				CloseHandle(handle);
				lineBuffer_.clear();
			}
			if (handle_ == INVALID_HANDLE_VALUE) {
				// 見つからなかった。少し待って探し直す（終了要求にはすぐ反応する）
				for (DWORD waited = 0; running_ && !reconnectRequest_ && waited < kRetryIntervalMs; waited += 50) {
					Sleep(50);
				}
			}
			continue;
		}

		// ---- 接続中：受信を続ける ----
		const uint32_t before = receivedLines_.load();
		readFailed_ = false;
		ReadLines(handle_, kReadTimeoutMs);
		if (readFailed_) {
			// ReadFile が失敗した（ケーブルが抜けた等）→ 閉じて探し直す
			ClosePort();
			continue;
		}
		const ULONGLONG now = GetTickCount64();
		if (receivedLines_.load() != before) {
			lastReceiveTick = now;
		}
		connected_ = (now - lastReceiveTick) < kConnectionTimeoutMs;
		if (!connected_ && now - lastReceiveTick > kConnectionTimeoutMs * 3) {
			// 開けてはいるが長く何も来ない（マイコンが止まった等）→ 開き直す
			ClosePort();
		}
	}
}

void TuboEngine::SliderInput::ShowDebugWindow() {
#ifdef USE_IMGUI
	if (!ImGuiManager::GetInstance()->BeginPanel("Slider Input")) {
		ImGuiManager::GetInstance()->EndPanel();
		return;
	}

	if (IsConnected()) {
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "接続中: %s", ToNarrow(GetConnectedPort()).c_str());
	} else {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "未接続（ポートを探しています）");
	}
	ImGui::Text("受信: %u 行/秒", linesPerSecond_);
	ImGui::Separator();

	for (int i = 0; i < kAxisCount; ++i) {
		char overlay[32];
		snprintf(overlay, sizeof(overlay), "%.2f  (raw %d)", value_[i], raw_[i]);
		ImGui::Text("Slider %d", i);
		ImGui::SameLine(80.0f);
		ImGui::ProgressBar(value_[i], ImVec2(-1.0f, 0.0f), overlay);
	}
	ImGui::Separator();

	// 範囲の調整。キャリブレーション中は、動かした範囲を最小/最大として覚える
	static bool calibrating = false;
	if (ImGui::Checkbox("キャリブレーション（全スライダーを端から端まで動かす）", &calibrating) && calibrating) {
		// 今の値から範囲を広げていく
		rawMin_ = raw_[0];
		rawMax_ = raw_[0];
	}
	if (calibrating && receivedLines_.load() > 0) {
		for (int i = 0; i < kAxisCount; ++i) {
			rawMin_ = (std::min)(rawMin_, raw_[i]);
			rawMax_ = (std::max)(rawMax_, raw_[i]);
		}
	}
	ImGui::DragIntRange2("Raw 範囲", &rawMin_, &rawMax_, 1.0f, 0, 65535);
	if (ImGui::SliderFloat("平滑化", &smoothing_, 0.0f, 0.95f)) {
		SetSmoothing(smoothing_);
	}
	ImGui::SliderFloat("端の不感帯", &edgeDeadZone_, 0.0f, 0.2f);
	ImGui::Separator();

	// ポート指定（空欄 = 自動検出）
	static char portText[16] = "";
	ImGui::InputText("ポート (例 COM3, 空=自動)", portText, sizeof(portText));
	if (ImGui::Button("接続し直す")) {
		std::wstring port;
		for (const char* p = portText; *p; ++p) {
			port += static_cast<wchar_t>(*p);
		}
		SetPort(port);
	}

	ImGuiManager::GetInstance()->EndPanel();
#endif // USE_IMGUI
}
