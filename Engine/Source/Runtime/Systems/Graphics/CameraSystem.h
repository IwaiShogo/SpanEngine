#pragma once
#include "ECS/Kernel/System.h"
#include "Application.h"
#include "Components/Core/Transform.h"
#include "Components/Core/LocalToWorld.h"
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

			GetWorld()->ForEach<Camera, LocalToWorld>(
				[&](Entity, Camera& cam, LocalToWorld& ltw)
				{
					// 1. View行列
					Matrix4x4 viewMatrix = ltw.Value.Invert();

					// 2. Projection行列
					float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
					if (aspectRatio <= 0.0f) aspectRatio = 1.0f;

					Matrix4x4 projMatrix = Matrix4x4::PerspectiveFovLH(
						ToRadians(cam.Fov),
						aspectRatio,
						cam.NearClip,
						cam.FarClip
					);

					// 3. レンダラーに適用
					renderer.SetCamera(viewMatrix, projMatrix);
				}
			);
		}
	};
}

