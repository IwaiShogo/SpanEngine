#pragma once
#include "ECS/Kernel/System.h"
#include "Application.h"
#include "Components/Core/Transform.h"
#include "Components/Graphics/Camera.h"

namespace Span
{
	class CameraSystem : public System
	{
	public:
		void OnUpdate() override
		{
			auto& renderer = Application::Get().GetRenderer();
			auto& window = Application::Get().GetWindow();
			float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());

			// カメラを持つEntityを探す
			bool cameraFound = false;

			GetWorld()->ForEach<Transform, Camera>(
				[&](Entity entity, Transform& transform, Camera& camera)
				{
					if (cameraFound) return;

					// 1. View行列の計算
					// Transformから行列取得
					Matrix4x4 worldMatrix = transform.GetWorldMatrix();
					XMMATRIX mWorld = worldMatrix.ToXM();

					// 逆行列を計算
					XMVECTOR det;
					XMMATRIX mView = XMMatrixInverse(&det, mWorld);

					// 2. Projection行列の計算
					XMMATRIX mProj = XMMatrixPerspectiveFovLH(
						ToRadians(camera.Fov),
						aspectRatio,
						camera.NearClip,
						camera.FarClip
					);

					// 3. Rendererにセット
					Matrix4x4 view, proj;
					view.FromXM(mView);
					proj.FromXM(mProj);

					renderer.SetCamera(view, proj);

					cameraFound = true;
				}
			);
		}
	};
}
