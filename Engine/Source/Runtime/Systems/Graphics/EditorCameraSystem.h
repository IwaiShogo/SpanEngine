/*****************************************************************//**
 * @file	EditorCameraSystem.h
 * @brief	エディタ用フリーカメラの操作システム
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "ECS/Kernel/System.h"
#include "Core/Input/Input.h"
#include "Components/Core/Transform.h"
#include "Components/Graphics/Camera.h"
#include "Components/Editor/EditorCamera.h"
#include "Core/Time/Time.h"
#include "Core/Math/SpanMath.h"

#include <imgui.h>

namespace Span
{
	/**
	 * @class	EditorCameraSystem
	 * @brief	🎥 エディタビューポート内のカメラ操作を処理するシステム。
	 *
	 * @details
	 * `EditorCamera` タグを持つエンティティに対し、以下の操作を提供します。
	 * - **右クリックホールド**: 視点回転 (Mouse Look)
	 * - **WASD**: 前後左右移動
	 * - **Q/E**: 下降/上昇
	 * - **Shift**: 高速移動
	 */
	class EditorCameraSystem : public System
	{
	private:
		float m_yaw = 0.0f;
		float m_pitch = 0.0f;
		bool m_controlling = false;

	public:
		void OnUpdate() override
		{
			World& world = Application::Get().GetWorld();
			float dt = Time::GetDeltaTime();

			world.ForEach<EditorCamera, Camera, Transform>(
				[&](Entity e, EditorCamera&, Camera& cam, Transform& trans)
				{
					// --- 操作開始/終了処理 ---
					// 右クリックが押されている間だけ操作可能
					bool isRightClicking = Input::GetKey(Key::MouseRight);

					if (isRightClicking && !m_controlling)
					{
						// 操作開始: カーソルをロックしている間だけ操作可能
						m_controlling = true;
						Input::SetLockCursor(true);

						// 現在のカメラの角度を yaw/pitch の初期値として取り込む
						Vector3 currentEuler = trans.Rotation.ToEuler();
						m_pitch = currentEuler.x;
						m_yaw = currentEuler.y;
					}
					else if (!isRightClicking && m_controlling)
					{
						// 操作終了: カーソルを解放
						m_controlling = false;
						Input::SetLockCursor(false);
					}

					// --- 操作ロジック (右クリック中のみ実行) ---
					if (m_controlling)
					{
						// 1. 視点回転 (Mouse Look)
						Vector2 delta = Input::GetMouseDelta();
						float sensitivity = 0.002f;

						// 値を加算
						m_yaw += delta.x * sensitivity;
						m_pitch += delta.y * sensitivity;

						// 角度制限 (-89 ~ 89度)
						float limit = ToRadians(89.0f);
						m_pitch = Clamp(m_pitch, -limit, limit);

						// 回転を再構築 (Rollを含まない)
						// 順序: Pitch(X) -> Yaw(Y) -> Roll(Z=0)
						trans.Rotation = Quaternion::FromEuler(m_pitch, m_yaw, 0.0f);

						// 2. 移動 (WASD + QE)
						float speed = 5.0f * dt;
						if (Input::GetKey(Key::LeftShift))
							speed *= 4.0f;	// Shiftで高速化

						Vector3 moveDir = Vector3::Zero;

						// カメラの前方・右方ベクトルを取得
						Vector3 forward = trans.GetForward();
						Vector3 rigtht = trans.GetRight();
						Vector3 up = Vector3::Up;	// ワールドY軸基準で上昇させる場合

						if (Input::GetKey(Key::W)) moveDir += forward;	// 前方
						if (Input::GetKey(Key::S)) moveDir -= forward;	// 後方
						if (Input::GetKey(Key::D)) moveDir += rigtht;	// 右方
						if (Input::GetKey(Key::A)) moveDir -= rigtht;	// 左方
						if (Input::GetKey(Key::E)) moveDir += up;		// 上昇
						if (Input::GetKey(Key::Q)) moveDir -= up;		// 下降

						if (moveDir.LengthSquared() > 0.001f)
						{
							moveDir.Normalized();
							trans.Position += moveDir * speed;
						}
					}
				}
			);
		}
	};
}
