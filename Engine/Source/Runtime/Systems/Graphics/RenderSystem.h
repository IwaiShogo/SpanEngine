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
			auto world = GetWorld();

			// --------------------------------------------------------
			// パス1: 不透明 (Opaque) の描画
			// 先に奥にある不透明な物体を描画して、深度バッファを埋めます。
			// --------------------------------------------------------
			world->ForEach<Transform, MeshRenderer>(
				[&](Entity entity, Transform& transform, MeshRenderer& meshRenderer)
				{
					if (!meshRenderer.MeshAsset || !meshRenderer.MaterialAsset) return;

					// 透明じゃないものだけ描く
					if (!meshRenderer.MaterialAsset->IsTransparent())
					{
						renderer.DrawMesh(meshRenderer.MeshAsset, meshRenderer.MaterialAsset, transform.GetWorldMatrix());
					}
				}
			);

			// --------------------------------------------------------
			// パス2: 透明 (Transparent) の描画
			// 不透明な物体の上に重ねて描画します（深度書き込みなし）。
			// --------------------------------------------------------
			world->ForEach<Transform, MeshRenderer>(
				[&](Entity entity, Transform& transform, MeshRenderer& meshRenderer)
				{
					if (!meshRenderer.MeshAsset || !meshRenderer.MaterialAsset) return;

					// 透明なものだけ描く
					if (meshRenderer.MaterialAsset->IsTransparent())
					{
						renderer.DrawMesh(meshRenderer.MeshAsset, meshRenderer.MaterialAsset, transform.GetWorldMatrix());
					}
				}
			);
		}
	};
}
