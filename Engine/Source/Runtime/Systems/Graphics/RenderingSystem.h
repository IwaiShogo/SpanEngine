#pragma once
#include "ECS/Kernel/System.h"
#include "ECS/Kernel/World.h"
#include "Runtime/Application.h"
#include "Graphics/Renderer.h"

// Components
#include "Components/Core/LocalToWorld.h"
#include "Components/Graphics/MeshFilter.h"
#include "Components/Graphics/MeshRenderer.h"

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
			world->ForEach<MeshFilter, MeshRenderer, LocalToWorld>(
				[&](MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw)
				{
					if (!mf.mesh || !mr.material) return;

					if (!mr.material->IsTransparent())
					{
						renderer.DrawMesh(mf.mesh, mr.material, ltw.Value);
					}
				}
			);

			// --------------------------------------------------------
			// パス2: 透明 (Transparent) の描画
			// 不透明な物体の上に重ねて描画します（深度書き込みなし）。
			// --------------------------------------------------------
			world->ForEach<MeshFilter, MeshRenderer, LocalToWorld>(
				[&](MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw)
				{
					if (!mf.mesh || !mr.material) return;

					if (mr.material->IsTransparent())
					{
						renderer.DrawMesh(mf.mesh, mr.material, ltw.Value);
					}
				}
			);
		}
	};
}
