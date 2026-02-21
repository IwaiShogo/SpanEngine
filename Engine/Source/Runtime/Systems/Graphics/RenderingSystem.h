/*****************************************************************//**
 * @file	RenderingSystem.h
 * @brief	描画コマンド発行システム。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

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
	/**
	 * @class	RenderingSystem
	 * @brief	🖌 シーン上の描画可能オブジェクトを収集し、Rendererへコマンドを発行するシステム。
	 *
	 * @details
	 * ECSのコンポーネント (Mesh, Material, Transform) を集め、レンダリングパイプラインへ渡します。
	 * 描画順序を制御するために、2角パスに分けて実行します。
	 */
	class RenderingSystem : public System
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
				[&](Entity, MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw)
				{
					if (!mf.mesh || !mr.material) return;

					if (mr.material->GetBlendMode() != BlendMode::Transparent)
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
				[&](Entity, MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw)
				{
					if (!mf.mesh || !mr.material) return;

					if (mr.material->GetBlendMode() == BlendMode::Transparent)
					{
						renderer.DrawMesh(mf.mesh, mr.material, ltw.Value);
					}
				}
			);
		}
	};
}

