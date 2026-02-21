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
#include "Runtime/Scene/Scene.h"
#include "Graphics/Renderer.h"

// Components
#include "Components/Core/LocalToWorld.h"
#include "Components/Graphics/MeshFilter.h"
#include "Components/Graphics/MeshRenderer.h"
#include "Components/Graphics/DirectionalLight.h"

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

			// 0. ライト情報の収集と送信
			Vector3 lightDir = { 0.0f, -1.0f, 1.0f };
			Vector3 lightColor = { 1.0f, 1.0f, 1.0f };
			float ambient = Application::Get().GetActiveScene().Environment.AmbientIntensity;

			world->ForEach<DirectionalLight, LocalToWorld>(
				[&](Entity, DirectionalLight& dl, LocalToWorld& ltw)
				{
					lightDir = Vector3(ltw.Value.m[2][0], ltw.Value.m[2][1], ltw.Value.m[2][2]);
					lightDir.Normalized();

					lightColor = dl.Color * dl.Intensity;
				}
			);
			renderer.SetGlobalLightData(lightDir, lightColor, ambient);

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
			// パス1.5: スカイボックス (Skybox) の描画
			// 不透明なオブジェクトで深度が書き込まれた後に行うことで、
			// 隠れている空のピクセルの描画をスキップし最適化します。
			// --------------------------------------------------------
			EnvironmentSettings& env = Application::Get().GetActiveScene().Environment;
			renderer.RenderSkybox(renderer.GetCommandList(), env);

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

