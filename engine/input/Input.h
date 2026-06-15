#pragma once

#include <Windows.h>
#include <XInput.h>
#include <array>
#include <cstdint>
#include <dinput.h>
#include <vector>
#include <wrl.h>
#include "engine/math/Vector2.h"

namespace TuboEngine {

class Input {
public:
	using Vector2 = Math::Vector2;

	struct MouseMove {
		LONG lX;
		LONG lY;
		LONG lZ;
	};

	enum class PadType {
		DirectInput,
		XInput,
	};

	union State {
		XINPUT_STATE xInput_;
		DIJOYSTATE2 directInput_;
		State() { ZeroMemory(this, sizeof(State)); }
	};

	struct Joystick {
		Microsoft::WRL::ComPtr<IDirectInputDevice8> device_;
		int32_t deadZoneL_ = 0;
		int32_t deadZoneR_ = 0;
		PadType type_;
		State state_;
		State statePre_;
		bool isConnected_ = false;
		int32_t xinputIndex_ = -1;
	};

public:
	static Input* GetInstance();

	void Initialize(HWND hwnd);
	void Update();
	void Finalize();
	void ShowInputDebugWindow();

	// キーボード
	bool PushKey(BYTE keyNumber) const;
	bool TriggerKey(BYTE keyNumber) const;
	const std::array<BYTE, 256>& GetAllKey() const { return key_; }

	// 入力のトリガーを消す（ポーズ解除などで、直前の押下判定を残さないため）
	void FlushTriggers();

	// マウス
	const DIMOUSESTATE2& GetAllMouse() const { return mouse_; }
	bool IsPressMouse(int32_t button) const;
	bool IsTriggerMouse(int32_t button) const;
	MouseMove GetMouseMove() const;
	int32_t GetWheel() const;
	const Vector2& GetMousePosition() const { return mousePosition_; }

	// ジョイスティック
	bool GetJoystickState(int32_t stickNo, DIJOYSTATE2& out) const;
	bool GetJoystickStatePrevious(int32_t stickNo, DIJOYSTATE2& out) const;
	bool GetJoystickState(int32_t stickNo, XINPUT_STATE& out) const;
	bool GetJoystickStatePrevious(int32_t stickNo, XINPUT_STATE& out) const;
	void SetJoystickDeadZone(int32_t stickNo, int32_t deadZoneL, int32_t deadZoneR);
	size_t GetNumberOfJoysticks() const;
	bool IsPadConnected(int32_t stickNo) const;

private:
	static Input* instance;
	Input() = default;
	~Input() = default;
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

	void SetupJoysticks();
	static BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) noexcept;

private:
	// DirectInput
	Microsoft::WRL::ComPtr<IDirectInput8> dInput_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> devKeyboard_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> devMouse_;
	std::vector<Joystick> devJoysticks_;

	// キーボード
	std::array<BYTE, 256> key_ = {};
	std::array<BYTE, 256> keyPre_ = {};

	// マウス
	DIMOUSESTATE2 mouse_ = {};
	DIMOUSESTATE2 mousePre_ = {};
	Vector2 mousePosition_ = {};

	// ウィンドウハンドル
	HWND hwnd_ = nullptr;
};

} // namespace TuboEngine
