#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Math/SpanMath.h"

namespace Span
{
	// キーコード定義
	enum class Key
	{
		None = 0,

		// Mouse
		MouseLeft, MouseRight, MouseMiddle,

		// Keyboard
		Space = 32,
		A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
		N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

		LeftShift = 160, RightShift,
		LeftControl, RightControl,
		Escape = 27,

		// Controller
		Gamepad_A, Gamepad_B, Gamepad_X, Gamepad_Y,
		Gamepad_Start, Gamepad_Back,
		Gamepad_LeftShoulder, Gamepad_RightShoulder,
		Gamepad_DPad_Up, Gamepad_DPad_Down, Gamepad_DPad_Left, Gamepad_DPad_Right
	};

	// アナログスティック
	enum class Axis
	{
		LeftStickX, LeftStickY,
		RightStickX, RightStickY,
		LeftTrigger, RightTrigger
	};

	class Input
	{
	public:
		static void Initialize();
		static void Update();

		// --- ユーザー向けAPI ---
		static bool GetKey(Key key);		// 押しっぱなし
		static bool GetKeyDown(Key key);	// 押した瞬間
		static bool GetKeyUp(Key key);		// 離した

		static Vector2 GetMousePosition();	// マウスの位置
		static Vector2 GetMouseDelta();		// マウスの移動量

		// コントローラー用関数
		static bool GetButton(Key key);		// ボタン押しっぱなし
		static bool GetButtonDown(Key key);	// 押した瞬間
		static float GetAxis(Axis axis);	// スティックの傾き (-1.0 ~ 1.0)

		// --- 内部用 (WindowProcから呼ぶ) ---
		static void OnKeyDown(uint32 key);
		static void OnKeyUp(uint32 key);
		static void OnMouseDown(uint32 btn);
		static void OnMouseUp(uint32 btn);
		static void OnMouseMove(int x, int y);

	private:
		// 現在のフレームのキー状態
		static bool keyStates[256];
		// 前フレームのキー状態 (Down / Up判定用)
		static bool prevKeyStates[256];

		static Vector2 mousePosition;
		static Vector2 prevMousePosition;
		static Vector2 mouseDelta;

		// コントローラー内部状態
		static bool gamepadStates[20];		// ボタン状態
		static bool prevGamepadStates[20];	// 前フレーム
		static float gamepadAxes[6];		// スティック/トリガー値
		static bool isConnected;			// 接続されているか
	};
}
