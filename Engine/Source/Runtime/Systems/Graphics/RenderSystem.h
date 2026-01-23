#pragma once
#include "ECS/Kernel/System.h"
#include "Application.h"
#include "Runtime/Components/Core/Transform.h"
#include "Runtime/Components/Graphics/GraphicsComponents.h"

namespace Span
{
	class RenderSystem : public System
	{
	public:
		void OnUpdate() override
		{
			// アプリケーションからレンダラーを取得
			Renderer& renderer = Application::Get().GetRenderer();

			GetWorld()->ForEach<Transform, MeshRenderer>(
				[&](Entity entity, Transform& transform, MeshRenderer& meshRenderer)
				{
					if (!meshRenderer.MeshAsset || !meshRenderer.MaterialAsset) return;

					Matrix4x4 worldMatrix = transform.GetWorldMatrix();

					renderer.DrawMesh(meshRenderer.MeshAsset, meshRenderer.MaterialAsset, worldMatrix);
				}
			);
		}
	};
}
