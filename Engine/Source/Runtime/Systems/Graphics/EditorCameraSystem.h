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
				[&](Entity e, EditorCamera& edCam, Camera& cam, Transform& trans)
				{
					// --- Input Status ---
					bool isRightClicking = Input::GetKey(Key::MouseRight);
					float wheel = ImGui::GetIO().MouseWheel;

					// フォーカスが無い限り入力を無視する
					if (!m_controlling && !edCam.IsFocused)
					{
						wheel = 0.0f;
						isRightClicking = false;
					}

					// 1. マウスホイール操作 (ズーム / 速度変更)
					// ============================================================
					if (wheel != 0.0f)
					{
						if (isRightClicking)
						{
							// 右クリック中: 移動速度の変更
							float change = wheel * (edCam.MoveSpeed * 0.1f);
							if (std::abs(change) < 0.1f) change = (wheel > 0) ? 0.1f : -0.1f;

							edCam.MoveSpeed += change;
							edCam.MoveSpeed = Clamp(edCam.MoveSpeed, 0.1f, 500.0f);
						}
						else
						{
							// 通常時: ズーム (前後移動 / Orthoサイズ)
							if (cam.Projection == ProjectionType::Orthographic)
							{
								// Ortho: サイズを変更 (ズームイン/アウト)
								cam.OrthographicSize -= wheel * edCam.ScrollSensitivity;
								cam.OrthographicSize = std::max(0.1f, cam.OrthographicSize);
							}
							else
							{
								// Persp: 前後移動
								Vector3 forward = trans.GetForward();
								trans.Position += forward * (wheel * edCam.ScrollSensitivity * 2.0f);
							}
						}
					}

					// 2. 右クリックでの視点操作開始/終了
					// ============================================================
					if (isRightClicking)
					{
						if (!m_controlling)
						{
							m_controlling = true;
							Input::SetLockCursor(true);
							Vector3 currentEuler = trans.Rotation.ToEuler();
							m_pitch = currentEuler.x;
							m_yaw = currentEuler.y;
						}

						// --- A. 視点回転 ---
						Vector2 delta = Input::GetMouseDelta();
						float sensitivity = 0.002f;

						m_yaw += delta.x * sensitivity;
						m_pitch += delta.y * sensitivity;

						float limit = ToRadians(89.0f);
						m_pitch = Clamp(m_pitch, -limit, limit);

						trans.Rotation = Quaternion::FromEuler(m_pitch, m_yaw, 0.0f);
					}
					else
					{
						if (m_controlling)
						{
							m_controlling = false;
							Input::SetLockCursor(false);
						}
					}

					// 3. 操作ロジック (慣性あり)
					// ============================================================
					Vector3 inputDir = Vector3::Zero;

					if (m_controlling)
					{
						// カメラの前方・右方ベクトルを取得
						Vector3 forward = trans.GetForward();
						Vector3 rigtht = trans.GetRight();
						Vector3 up = Vector3::Up;	// ワールドY軸基準で上昇させる場合

						if (Input::GetKey(Key::W)) inputDir += forward;	// 前方
						if (Input::GetKey(Key::S)) inputDir -= forward;	// 後方
						if (Input::GetKey(Key::D)) inputDir += rigtht;	// 右方
						if (Input::GetKey(Key::A)) inputDir -= rigtht;	// 左方
						if (Input::GetKey(Key::E)) inputDir += up;		// 上昇
						if (Input::GetKey(Key::Q)) inputDir -= up;		// 下降

						if (inputDir.LengthSquared() > 0.001f)
							inputDir = inputDir.Normalized();
					}

					// 速度計算
					if (inputDir.LengthSquared() > 0.001f)
					{
						// 入力がある場合: 加速
						float targetSpeed = edCam.MoveSpeed;
						if (Input::GetKey(Key::LeftShift)) targetSpeed *= edCam.SprintMultiplier;

						// 現在の速度ベクトルに入力方向への加速を加える
						Vector3 targetVelocity = inputDir * targetSpeed;

						// Lerpで加速
						edCam.Velocity = Vector3::Lerp(edCam.Velocity, targetVelocity, edCam.Acceleration * dt);
					}
					else
					{
						// 入力が無い場合: インスタントストップ
						edCam.Velocity = Vector3::Zero;
					}

					// 位置の更新
					trans.Position += edCam.Velocity * dt;
				}
			);
		}
	};
}
