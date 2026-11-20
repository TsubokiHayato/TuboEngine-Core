#pragma once

#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace TuboEngine {

/// <summary>
/// マイコン（PlatformIO）の4スライダーの値を USB シリアルで受け取るクラス（シングルトン）。
/// マイコンは "  12,  345,  678,  910" のような1行を 115200bps で送ってくる前提。
/// 受信は別スレッドで行い、ゲーム側は Update() 後に Get() で最新値を取るだけ（待ちは発生しない）。
/// ポート未指定なら COM1〜COM32 を自動で探し、抜けたら自動で再接続する。
/// </summary>
class SliderInput {
public:
	static constexpr int kAxisCount = 4;

	static SliderInput* GetInstance();

	/// <summary>
	/// 受信スレッドを開始する。portName は L"COM3" など。空なら自動検出。
	/// </summary>
	void Initialize(const std::wstring& portName = L"", DWORD baudRate = 115200);
	/// <summary>
	/// 毎フレーム1回呼ぶ。受信済みの最新値をこのフレームの値として確定させる。
	/// </summary>
	void Update();
	/// <summary>
	/// 受信スレッドを止めてポートを閉じる。
	/// </summary>
	void Finalize();
	/// <summary>
	/// ImGui のデバッグウィンドウ（値・接続状態・範囲調整）を表示する。
	/// </summary>
	void ShowDebugWindow();

	/// <summary>マイコンとつながっていて、値が届いているか。</summary>
	bool IsConnected() const { return connected_.load(); }
	/// <summary>0.0〜1.0 に正規化・平滑化した値。</summary>
	float Get(int axis) const;
	/// <summary>前フレームの Get() の値。</summary>
	float GetPrevious(int axis) const;
	/// <summary>このフレームでの変化量（Get - GetPrevious）。</summary>
	float GetDelta(int axis) const { return Get(axis) - GetPrevious(axis); }
	/// <summary>マイコンから届いた生の値。</summary>
	int GetRaw(int axis) const;

	/// <summary>生の値の範囲（この範囲を 0〜1 にする）。既定は 0〜1023。</summary>
	void SetRawRange(int rawMin, int rawMax) { rawMin_ = rawMin; rawMax_ = rawMax; }
	/// <summary>平滑化の強さ 0.0（なし）〜0.95（強い）。値のガタつきを抑える。</summary>
	void SetSmoothing(float smoothing);
	/// <summary>両端の不感帯（0〜1 の割合）。端まで動かしても 0/1 にならない場合に使う。</summary>
	void SetEdgeDeadZone(float deadZone) { edgeDeadZone_ = deadZone; }
	/// <summary>指定ポートに切り替える（空なら自動検出）。</summary>
	void SetPort(const std::wstring& portName);
	/// <summary>接続中のポート名（未接続なら空）。</summary>
	std::wstring GetConnectedPort() const;

private:
	SliderInput() = default;
	~SliderInput() = default;
	SliderInput(const SliderInput&) = delete;
	SliderInput& operator=(const SliderInput&) = delete;

	void ThreadMain();
	HANDLE OpenPort(const std::wstring& portName) const;
	// 開いたポートから1行読めるまで待ち、4つの値が読めたら true（自動検出の確認用）
	bool ReadLines(HANDLE handle, DWORD timeoutMs);
	bool ParseLine(const std::string& line);
	void ClosePort();

private:
	static SliderInput* instance;

	// 受信スレッド
	std::thread thread_;
	std::atomic<bool> running_{false};
	std::atomic<bool> connected_{false};
	std::atomic<bool> reconnectRequest_{false};
	HANDLE handle_ = INVALID_HANDLE_VALUE;
	DWORD baudRate_ = 115200;
	bool readFailed_ = false; // 受信スレッド専用
	std::string lineBuffer_;

	// ポート名（メインスレッドと受信スレッドの両方から触るので mutex で守る）
	mutable std::mutex portMutex_;
	std::wstring requestedPort_;
	std::wstring connectedPort_;

	// 受信スレッドが書く最新の生の値
	std::array<std::atomic<int>, kAxisCount> latestRaw_{};
	std::atomic<uint32_t> receivedLines_{0};

	// Update() で確定させた、このフレームの値
	std::array<int, kAxisCount> raw_{};
	std::array<float, kAxisCount> value_{};
	std::array<float, kAxisCount> valuePre_{};
	bool hasValue_ = false;

	// 設定
	int rawMin_ = 0;
	int rawMax_ = 1023;
	float smoothing_ = 0.5f;
	float edgeDeadZone_ = 0.02f;
	// デバッグ表示用（受信頻度）
	uint32_t linesPerSecond_ = 0;
	uint32_t lineCountAtLastSecond_ = 0;
	ULONGLONG lastSecondTick_ = 0;
};

} // namespace TuboEngine
