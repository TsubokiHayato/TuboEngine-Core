#include "Input.h"
#include "ImGuiManager.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xinput.lib")

TuboEngine::Input* TuboEngine::Input::instance = nullptr;
TuboEngine::Input* TuboEngine::Input::GetInstance() {
	if (instance == nullptr) {
		instance = new Input();
	}
	return instance;
}

void TuboEngine::Input::Initialize(HWND hwnd) {
	hwnd_ = hwnd;

	HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(dInput_.GetAddressOf()), nullptr);
	assert(SUCCEEDED(hr));

	// キーボード
	hr = dInput_->CreateDevice(GUID_SysKeyboard, &devKeyboard_, nullptr);
	assert(SUCCEEDED(hr));
	hr = devKeyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));
	hr = devKeyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(hr));

	// マウス
	hr = dInput_->CreateDevice(GUID_SysMouse, &devMouse_, nullptr);
	assert(SUCCEEDED(hr));
	hr = devMouse_->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(hr));
	hr = devMouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(hr));

	// キー・マウス初期化
	key_.fill(0);
	keyPre_.fill(0);
	ZeroMemory(&mouse_, sizeof(mouse_));
	ZeroMemory(&mousePre_, sizeof(mousePre_));

	// ジョイスティック列挙
	SetupJoysticks();
}

void TuboEngine::Input::Update() {
	// キーボード
	keyPre_ = key_;
	HRESULT hr = devKeyboard_->Acquire();
	if (SUCCEEDED(hr)) {
		hr = devKeyboard_->GetDeviceState(sizeof(key_), key_.data());
		if (FAILED(hr)) {
			key_.fill(0);
		}
	} else {
		key_.fill(0);
	}

	// マウス
	mousePre_ = mouse_;
	hr = devMouse_->Acquire();
	if (SUCCEEDED(hr)) {
		hr = devMouse_->GetDeviceState(sizeof(mouse_), &mouse_);
		if (FAILED(hr)) {
			ZeroMemory(&mouse_, sizeof(mouse_));
		}
	} else {
		ZeroMemory(&mouse_, sizeof(mouse_));
	}

	// マウス座標（ウィンドウ座標系）
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hwnd_, &pt);
	mousePosition_.x = static_cast<float>(pt.x);
	mousePosition_.y = static_cast<float>(pt.y);

	// ジョイスティック接続状態・入力更新
	for (size_t i = 0; i < devJoysticks_.size(); ++i) {
		auto& joystick = devJoysticks_[i];
		joystick.statePre_ = joystick.state_;
		if (joystick.type_ == PadType::DirectInput) {
			HRESULT hr = joystick.device_->Acquire();
			if (SUCCEEDED(hr)) {
				hr = joystick.device_->GetDeviceState(sizeof(DIJOYSTATE2), &joystick.state_.directInput_);
				joystick.isConnected_ = SUCCEEDED(hr);
			} else {
				joystick.isConnected_ = false;
			}
		} else if (joystick.type_ == PadType::XInput) {
			DWORD result = XInputGetState(static_cast<DWORD>(i), &joystick.state_.xInput_);
			joystick.isConnected_ = (result == ERROR_SUCCESS);
		}
	}

	// 必要に応じて新規接続の再列挙（例: 一定フレームごとにSetupJoysticks()を呼ぶ）
}

void TuboEngine::Input::FlushTriggers() {
	// 現在→前回へ同期することで、Trigger判定を出さない
	keyPre_ = key_;
	mousePre_ = mouse_;
	for (auto& joystick : devJoysticks_) {
		joystick.statePre_ = joystick.state_;
	}
}

void TuboEngine::Input::Finalize() {
	if (devKeyboard_) {
		devKeyboard_->Unacquire();
		devKeyboard_.Reset();
	}
	if (devMouse_) {
		devMouse_->Unacquire();
		devMouse_.Reset();
	}
	if (dInput_) {
		dInput_.Reset();
	}
	for (auto& joystick : devJoysticks_) {
		if (joystick.device_) {
			joystick.device_->Unacquire();
			joystick.device_.Reset();
		}
	}
	devJoysticks_.clear();
	delete instance;
	instance = nullptr;
}

void TuboEngine::Input::ShowInputDebugWindow() {

#ifdef USE_IMGUI
	Input* input = Input::GetInstance();

	ImGui::Begin("Input Debug");

	// キーボード
	ImGui::Text("Keyboard");
	ImGui::Separator();
	for (int i = 0; i < 256; ++i) {
		if (input->PushKey(static_cast<BYTE>(i))) {
			ImGui::Text("Key %d (0x%02X): Down", i, i);
		}
	}

	// マウス
	ImGui::Text("Mouse");
	ImGui::Separator();
	static const char* mouseBtnNames[8] = {"Left", "Right", "Middle", "Button 4", "Button 5", "Button 6", "Button 7", "Button 8"};
	for (int i = 0; i < 8; ++i) {
		if (input->IsPressMouse(i)) {
			ImGui::Text("Mouse %s: Down", mouseBtnNames[i]);
		}
	}
	Input::MouseMove move = input->GetMouseMove();
	ImGui::Text("Move: X=%ld, Y=%ld, Z=%ld", move.lX, move.lY, move.lZ);
	ImGui::Text("Wheel: %d", input->GetWheel());
	Vector2 pos = input->GetMousePosition();
	ImGui::Text("Position: X=%.1f, Y=%.1f", pos.x, pos.y);

	// ジョイスティック
	ImGui::Text("Joysticks: %zu", input->GetNumberOfJoysticks());
	for (size_t i = 0; i < input->GetNumberOfJoysticks(); ++i) {
		bool connected = input->IsPadConnected(static_cast<int32_t>(i));
		ImGui::Text("Joystick %zu: %s", i, connected ? "Connected" : "Disconnected");
		if (!connected)
			continue;

		// パッドタイプごとに表示
		 auto& joystick = input->devJoysticks_[i];
		if (joystick.type_ == Input::PadType::DirectInput) {
			DIJOYSTATE2 joy;
			if (input->GetJoystickState(static_cast<int32_t>(i), joy)) {
				ImGui::Text("  [DirectInput] X=%ld Y=%ld Z=%ld", joy.lX, joy.lY, joy.lZ);
				for (int b = 0; b < 8; ++b) {
					if (joy.rgbButtons[b] & 0x80) {
						ImGui::Text("    Button %d: Down", b);
					}
					LONG rightStickX = joy.lRx;
					LONG rightStickY = joy.lRy;
					// rightStickX, rightStickYが右スティックの値
				}
			}
		} else if (joystick.type_ == PadType::XInput) {
			// devJoysticks_のインデックスではなくxinputIndex_を使う
			DWORD result = XInputGetState(static_cast<DWORD>(joystick.xinputIndex_), &joystick.state_.xInput_);
            joystick.isConnected_ = (result == ERROR_SUCCESS) ? true : false;
		}
	}

	ImGui::End();

	#endif // USE_IMGUI
}

// キーボード
bool TuboEngine::Input::PushKey(BYTE keyNumber) const { return (key_[keyNumber] & 0x80) != 0; }

bool TuboEngine::Input::TriggerKey(BYTE keyNumber) const { return ((key_[keyNumber] & 0x80) != 0) && ((keyPre_[keyNumber] & 0x80) == 0); }

// マウス
bool TuboEngine::Input::IsPressMouse(int32_t button) const {
	if (button < 0 || button >= 8)
		return false;
	return (mouse_.rgbButtons[button] & 0x80) != 0;
}

bool TuboEngine::Input::IsTriggerMouse(int32_t button) const {
	if (button < 0 || button >= 8)
		return false;
	return ((mouse_.rgbButtons[button] & 0x80) != 0) && ((mousePre_.rgbButtons[button] & 0x80) == 0);
}

TuboEngine::Input::MouseMove TuboEngine::Input::GetMouseMove() const {
	MouseMove move;
	move.lX = mouse_.lX;
	move.lY = mouse_.lY;
	move.lZ = mouse_.lZ;
	return move;
}

int32_t TuboEngine::Input::GetWheel() const { return static_cast<int32_t>(mouse_.lZ); }

// --- ジョイスティック関連は必要に応じて実装してください ---
bool TuboEngine::Input::GetJoystickState(int32_t stickNo, DIJOYSTATE2& out) const {
	if (stickNo < 0 || stickNo >= static_cast<int32_t>(devJoysticks_.size()))
		return false;
	out = devJoysticks_[stickNo].state_.directInput_;
	return true;
}

bool TuboEngine::Input::GetJoystickStatePrevious(int32_t stickNo, DIJOYSTATE2& out) const {
	if (stickNo < 0 || stickNo >= static_cast<int32_t>(devJoysticks_.size()))
		return false;
	out = devJoysticks_[stickNo].statePre_.directInput_;
	return true;
}

bool TuboEngine::Input::GetJoystickState(int32_t stickNo, XINPUT_STATE& out) const {
	if (stickNo < 0 || stickNo >= static_cast<int32_t>(devJoysticks_.size()))
		return false;
	out = devJoysticks_[stickNo].state_.xInput_;
	return true;
}

bool TuboEngine::Input::GetJoystickStatePrevious(int32_t stickNo, XINPUT_STATE& out) const {
	if (stickNo < 0 || stickNo >= static_cast<int32_t>(devJoysticks_.size()))
		return false;
	out = devJoysticks_[stickNo].statePre_.xInput_;
	return true;
}

void TuboEngine::Input::SetJoystickDeadZone(int32_t stickNo, int32_t deadZoneL, int32_t deadZoneR) {
	if (stickNo < 0 || stickNo >= static_cast<int32_t>(devJoysticks_.size()))
		return;
	devJoysticks_[stickNo].deadZoneL_ = deadZoneL;
	devJoysticks_[stickNo].deadZoneR_ = deadZoneR;
}

size_t TuboEngine::Input::GetNumberOfJoysticks() const { return devJoysticks_.size(); }

bool TuboEngine::Input::IsPadConnected(int32_t stickNo) const {
	if (stickNo < 0 || stickNo >= static_cast<int32_t>(devJoysticks_.size()))
		return false;
	return devJoysticks_[stickNo].isConnected_;
}

// --- ジョイスティック列挙 ---
BOOL CALLBACK TuboEngine::Input::EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) noexcept {
	auto* input = reinterpret_cast<Input*>(pContext);

	Microsoft::WRL::ComPtr<IDirectInputDevice8> device;
	HRESULT hr = input->dInput_->CreateDevice(pdidInstance->guidInstance, &device, nullptr);
	if (FAILED(hr))
		return DIENUM_CONTINUE;

	hr = device->SetDataFormat(&c_dfDIJoystick2);
	if (FAILED(hr))
		return DIENUM_CONTINUE;
	hr = device->SetCooperativeLevel(input->hwnd_, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(hr))
		return DIENUM_CONTINUE;

	Joystick joystick;
	joystick.device_ = device;
	joystick.type_ = PadType::DirectInput;
	joystick.isConnected_ = true;

	input->devJoysticks_.push_back(std::move(joystick));
	return DIENUM_CONTINUE;
}

void TuboEngine::Input::SetupJoysticks() {
	devJoysticks_.clear();
	if (dInput_) {
		dInput_->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, this, DIEDFL_ATTACHEDONLY);
	}
	// XInputパッドの重複追加を防ぐ
	for (DWORD i = 0; i < 4; ++i) {
		XINPUT_STATE xstate = {};
		if (XInputGetState(i, &xstate) == ERROR_SUCCESS) {
			// すでに同じXInputインデックスのパッドが追加されていないかチェック
			bool alreadyAdded = false;
			for (const auto& joy : devJoysticks_) {
				if (joy.type_ == PadType::XInput && joy.xinputIndex_ == static_cast<int32_t>(i)) {
					alreadyAdded = true;
					break;
				}
			}
			if (alreadyAdded)
				continue;
			Joystick joystick;
			joystick.type_ = PadType::XInput;
			joystick.isConnected_ = true;
			joystick.state_.xInput_ = xstate;
			joystick.xinputIndex_ = static_cast<int32_t>(i);
			devJoysticks_.push_back(joystick);
		}
	}
}
