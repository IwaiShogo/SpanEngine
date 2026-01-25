#pragma once
#include "ECS/Kernel/System.h"
#include "Core/Input/Input.h"
#include "Components/Core/Transform.h"
#include "Components/Graphics/Camera.h"
#include "Core/Time/Time.h"
#include "Core/Math/SpanMath.h"

namespace Span
{
	class EditorCameraSystem : public System
	{
	private:
		// 現在の角度を保持 (ラジアン)
		float m_yaw = 0.0f;
		float m_pitch = 0.0f;
		bool m_isInitialized = false;

	public:
		void OnUpdate() override
		{
			GetWorld()->ForEach<Camera, Transform>(
				[&](Entity, Camera& cam, Transform& trans)
				{
					// 初回のみ現在の回転から角度を推定（簡易的にリセット）
					if (!m_isInitialized)
					{
						// 今回は簡易的に0リセットしますが、本来はLookAtの向きから逆算します
						m_yaw = 0.0f;
						m_pitch = 0.0f;
						m_isInitialized = true;
					}

					float dt = Time::GetDeltaTime();

					// --- 1. 回転 (右クリック中のみ) ---
					if (Input::GetKey(Key::MouseRight))
					{
						Vector2 delta = Input::GetMouseDelta();
						float sensitivity = 0.002f;

						// 値を加算
						m_yaw += delta.x * sensitivity;
						m_pitch += delta.y * sensitivity;

						// 上下の角度制限 (真上・真下付近で止める: -89度 ~ 89度)
						float limit = ToRadians(89.0f);
						m_pitch = Clamp(m_pitch, -limit, limit);

						// 回転を再構築 (Rollを含まない)
						// 順序: Pitch(X) -> Yaw(Y) -> Roll(Z=0)
						trans.Rotation = Quaternion::FromEuler(m_pitch, m_yaw, 0.0f);
					}

					// --- 2. 移動 (ローカル座標基準) ---
					float speed = 10.0f * dt; // 少し速くしました
					if (Input::GetKey(Key::LeftShift)) speed *= 4.0f;

					Vector3 moveDir = Vector3::Zero;

					// カメラの現在の向きに合わせて移動ベクトルを作成
					// Forward/Rightは現在のRotationから計算されるため、視線方向に正しく進みます
					if (Input::GetKey(Key::W)) moveDir += trans.GetForward();
					if (Input::GetKey(Key::S)) moveDir -= trans.GetForward();
					if (Input::GetKey(Key::D)) moveDir += trans.GetRight();
					if (Input::GetKey(Key::A)) moveDir -= trans.GetRight();

					// 上昇・下降 (ワールドY軸)
					if (Input::GetKey(Key::E)) moveDir += Vector3::Up;
					if (Input::GetKey(Key::Q)) moveDir -= Vector3::Up;

					trans.Position += moveDir * speed;
				}
			);
		}
	};
}