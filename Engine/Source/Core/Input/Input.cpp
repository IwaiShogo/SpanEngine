#include "Input.h"
#include <Xinput.h>

#pragma comment(lib, "xinput.lib")

namespace Span
{
	// 静的メンバの定義
	bool Input::keyStates[256] = { false };
	bool Input::prevKeyStates[256] = { false };
	Vector2 Input::mousePosition = { 0, 0 };
	Vector2 Input::prevMousePosition = { 0, 0 };
	Vector2 Input::mouseDelta = { 0, 0 };

	// コントローラー用
	bool Input::gamepadStates[20] = { false };
	bool Input::prevGamepadStates[20] = { false };
	float Input::gamepadAxes[6] = { 0.0f };
	bool Input::isConnected = false;

	// ボタンと配列インデックスの対応表マップ
	int GetGamepadIndex(Key key)
	{
		switch (key)
		{
		case Key::Gamepad_A: return 0;
		case Key::Gamepad_B: return 1;
		case Key::Gamepad_X: return 2;
		case Key::Gamepad_Y: return 3;
		case Key::Gamepad_Start: return 4;
		case Key::Gamepad_Back: return 5;
		case Key::Gamepad_LeftShoulder: return 6;
		case Key::Gamepad_RightShoulder: return 7;
		case Key::Gamepad_DPad_Up: return 8;
		case Key::Gamepad_DPad_Down: return 9;
		case Key::Gamepad_DPad_Left: return 10;
		case Key::Gamepad_DPad_Right: return 11;
		default: return -1;
		}
	}

	void Input::Initialize()
	{
		// マウス初期位置を取得
		POINT p;
		GetCursorPos(&p);
		mousePosition = { (float)p.x, (float)p.y };
		prevMousePosition = mousePosition;
	}

	void Input::Update()
	{
		// 1. キーボード・マウスの前回状態保存
		memcpy(prevKeyStates, keyStates, sizeof(bool) * 256);
		mouseDelta = mousePosition - prevMousePosition;
		prevMousePosition = mousePosition;

		// 2. コントローラーの前回状態保存
		memcpy(prevGamepadStates, gamepadStates, sizeof(bool) * 20);

		// 3. XInputポーリング (コントローラー0番のみ対応)
		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));

		// 接続確認と取得
		if (XInputGetState(0, &state) == ERROR_SUCCESS)
		{
			isConnected = true;

			// --- ボタンの読み取り ---
			auto CheckBtn = [&](int idx, int mask) { gamepadStates[idx] = (state.Gamepad.wButtons & mask) != 0; };
			CheckBtn(0, XINPUT_GAMEPAD_A);
			CheckBtn(1, XINPUT_GAMEPAD_B);
			CheckBtn(2, XINPUT_GAMEPAD_X);
			CheckBtn(3, XINPUT_GAMEPAD_Y);
			CheckBtn(4, XINPUT_GAMEPAD_START);
			CheckBtn(5, XINPUT_GAMEPAD_BACK);
			CheckBtn(6, XINPUT_GAMEPAD_LEFT_SHOULDER);
			CheckBtn(7, XINPUT_GAMEPAD_RIGHT_SHOULDER);
			CheckBtn(8, XINPUT_GAMEPAD_DPAD_UP);
			CheckBtn(9, XINPUT_GAMEPAD_DPAD_DOWN);
			CheckBtn(10, XINPUT_GAMEPAD_DPAD_LEFT);
			CheckBtn(11, XINPUT_GAMEPAD_DPAD_RIGHT);

			// --- スティックの読み取りとデッドゾーン処理 ---
			auto ApplyDeadzone = [](short val) -> float {
				if (std::abs(val) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) return 0.0f;
				return val / 32767.0f;
				};

			gamepadAxes[(int)Axis::LeftStickX] = ApplyDeadzone(state.Gamepad.sThumbLX);
			gamepadAxes[(int)Axis::LeftStickY] = ApplyDeadzone(state.Gamepad.sThumbLY);
			gamepadAxes[(int)Axis::RightStickX] = ApplyDeadzone(state.Gamepad.sThumbRX);
			gamepadAxes[(int)Axis::RightStickY] = ApplyDeadzone(state.Gamepad.sThumbRY);

			// トリガー (0-255 -> 0.0-1.0)
			gamepadAxes[(int)Axis::LeftTrigger] = state.Gamepad.bLeftTrigger / 255.0f;
			gamepadAxes[(int)Axis::RightTrigger] = state.Gamepad.bRightTrigger / 255.0f;
		}
		else
		{
			isConnected = false;
			ZeroMemory(gamepadStates, sizeof(bool) * 20);
			ZeroMemory(gamepadAxes, sizeof(float) * 6);
		}
	}

	bool Input::GetKey(Key key) { return keyStates[(int)key]; }
	bool Input::GetKeyDown(Key key) { return keyStates[(int)key] && !prevKeyStates[(int)key]; }
	bool Input::GetKeyUp(Key key) { return !keyStates[(int)key] && prevKeyStates[(int)key]; }

	Vector2 Input::GetMousePosition() { return mousePosition; }
	Vector2 Input::GetMouseDelta() { return mouseDelta; }

	// コントローラーAPI
	bool Input::GetButton(Key key)
	{
		int idx = GetGamepadIndex(key);
		return (idx >= 0) ? gamepadStates[idx] : false;
	}
	bool Input::GetButtonDown(Key key)
	{
		int idx = GetGamepadIndex(key);
		return (idx >= 0) ? (gamepadStates[idx] && !prevGamepadStates[idx]) : false;
	}
	float Input::GetAxis(Axis axis) { return gamepadAxes[(int)axis]; }

	// --- WindowProcからのイベント ---
	void Input::OnKeyDown(uint32 key) { if (key < 256) keyStates[key] = true; }
	void Input::OnKeyUp(uint32 key) { if (key < 256) keyStates[key] = false; }
	void Input::OnMouseDown(uint32 btn)
	{
		if (btn == 0) keyStates[(int)Key::MouseLeft] = true;
		if (btn == 1) keyStates[(int)Key::MouseRight] = true;
		if (btn == 2) keyStates[(int)Key::MouseMiddle] = true;
	}
	void Input::OnMouseUp(uint32 btn)
	{
		if (btn == 0) keyStates[(int)Key::MouseLeft] = false;
		if (btn == 1) keyStates[(int)Key::MouseRight] = false;
		if (btn == 2) keyStates[(int)Key::MouseMiddle] = false;
	}
	void Input::OnMouseMove(int x, int y)
	{
		mousePosition = { (float)x, (float)y };
	}
}