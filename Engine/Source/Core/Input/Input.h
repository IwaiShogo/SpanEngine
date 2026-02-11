/*****************************************************************//**
 * @file	Input.h
 * @brief	入力管理システム (キーボード、マウス、ゲームパッド)。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Math/SpanMath.h"

namespace Span
{
	/// @brief	仮想キーコード (Virtual Key Codes)
	enum class Key
	{
		None = 0,

		// --- Mouse ---
		MouseLeft, MouseRight, MouseMiddle,

		// --- Keyboard ---
		Space = 32,
		A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
		N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

		LeftShift = 160, RightShift,
		LeftControl, RightControl,
		Escape = 27,

		// --- Gamepad (XInput) ---
		Gamepad_A, Gamepad_B, Gamepad_X, Gamepad_Y,
		Gamepad_Start, Gamepad_Back,
		Gamepad_LeftShoulder, Gamepad_RightShoulder,
		Gamepad_DPad_Up, Gamepad_DPad_Down, Gamepad_DPad_Left, Gamepad_DPad_Right
	};

	/// @brief	アナログ軸
	enum class Axis
	{
		LeftStickX, LeftStickY,
		RightStickX, RightStickY,
		LeftTrigger, RightTrigger
	};

	/**
	 * @class	Input
	 * @brief	🎮 入力状態をポーリングする静的クラス。
	 * 
	 * @details
	 * `Update()` を毎フレーム呼び出すことで、マイフレームとの差分 (Down/Up) を計算します。
	 * 
	 * ### 📝 Usage
	 * ```cpp
	 * if (Input::GetKeyDown(Key::Space)) { Jump(); }
	 * flaot move = Input::GetAxis(Axis::LeftStickX);
	 * ```
	 */
	class Input
	{
	public:
		static void Initialize(HWND hWnd);
		static void Update();
		static void EndFrame();

		// 🔍 Polling API
		// ============================================================

		/// @brief	指定したキーが現在押されているか
		static bool GetKey(Key key);

		/// @brief	指定したキーが「このフレームで」押されたか
		static bool GetKeyDown(Key key);

		/// @brief	指定したキーが「このフレームで」離されたか
		static bool GetKeyUp(Key key);

		// --- Mouse ---
		/// @brief	マウスの位置を取得します。
		static Vector2 GetMousePosition();

		/// @brief	マウスの移動量を取得します。
		static Vector2 GetMouseDelta();

		/// @brief	ホイール移動量を取得します。
		static float GetMouseWheel();

		/// @brief	カーソルの表示を設定します。
		static void SetCursorVisible(bool visible);

		/// @brief	カーソルを画面中央にロックし、非表示にします (FPS視点用)
		static void SetLockCursor(bool lock);

		// --- Gamepad ---
		/// @brief	指定したボタンが現在押されているか
		static bool GetButton(Key key);

		/// @brief	指定したボタンが「このフレームで」押されたか
		static bool GetButtonDown(Key key);

		/// @brief 指定したスティックの傾きを取得します。 (-1.0 ~ 1.0)
		static float GetAxis(Axis axis);

		// --- 内部用 (WindowProcから呼ぶ) ---
		static void OnKeyDown(uint32 key);
		static void OnKeyUp(uint32 key);
		static void OnMouseDown(uint32 btn);
		static void OnMouseUp(uint32 btn);
		static void OnMouseMove(int x, int y);
		static void OnMouseWheel(float delta);

		/// @brief	ImGuiがマウスを占有しているか設定する
		static void SetImGuiWantCapture(bool wantCapture) { imGuiWantCaptureMouse = wantCapture; }
		/// @brief	UIが入力を奪っていないか
		static bool IsInputAvailable() { return !imGuiWantCaptureMouse; }

	private:
		// State
		// 現在のフレームのキー状態
		static bool keyStates[256];
		// 前フレームのキー状態 (Down / Up判定用)
		static bool prevKeyStates[256];

		static Vector2 mousePosition;
		static Vector2 prevMousePosition;
		static Vector2 mouseDelta;
		static float mouseWheelDelta;

		static HWND hWnd;
		static bool isCursorLocked;
		static bool ignoreNextDelta;

		// Gamepad
		// コントローラー内部状態
		static bool gamepadStates[20];		// ボタン状態
		static bool prevGamepadStates[20];	// 前フレーム
		static float gamepadAxes[6];		// スティック/トリガー値
		static bool isConnected;			// 接続されているか
		static void ResetCursorToCenter();

		static bool imGuiWantCaptureMouse;
	};
}

