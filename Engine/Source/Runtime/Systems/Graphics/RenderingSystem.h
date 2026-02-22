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
#include "Components/Graphics/PointLight.h"
#include "Components/Graphics/SpotLight.h"

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

			EnvironmentSettings& env = Application::Get().GetActiveScene().Environment;

			// 1. 全ライト情報を収集するリスト
			std::vector<LightDataGPU> activeLights;

			// A. Directional Light
			world->ForEach<DirectionalLight, LocalToWorld>(
				[&](Entity, DirectionalLight& dl, LocalToWorld& ltw)
				{
					LightDataGPU ld = {};
					ld.Type = 0;
					ld.Direction = Vector3::Normalize(Vector3(ltw.Value.m[2][0], ltw.Value.m[2][1], ltw.Value.m[2][2]));
					ld.Color = dl.Color;
					ld.Intensity = dl.Intensity;
					activeLights.push_back(ld);
				}
			);

			// B. Point Light
			world->ForEach<PointLight, LocalToWorld>(
				[&](Entity, PointLight& pl, LocalToWorld& ltw)
				{
					LightDataGPU ld = {};
					ld.Type = 1;
					ld.Position = Vector3(ltw.Value.m[3][0], ltw.Value.m[3][1], ltw.Value.m[3][2]);
					ld.Color = pl.Color;
					ld.Intensity = pl.Intensity;
					ld.Range = pl.Range;
					activeLights.push_back(ld);
				}
			);

			// C. Spot Light
			world->ForEach<SpotLight, LocalToWorld>(
				[&](Entity, SpotLight& sl, LocalToWorld& ltw)
				{
					LightDataGPU ld = {};
					ld.Type = 2;
					ld.Position = Vector3(ltw.Value.m[3][0], ltw.Value.m[3][1], ltw.Value.m[3][2]);
					ld.Direction = Vector3::Normalize(Vector3(ltw.Value.m[2][0], ltw.Value.m[2][1], ltw.Value.m[2][2]));
					ld.Color = sl.Color;
					ld.Intensity = sl.Intensity;
					ld.Range = sl.Range;
					ld.InnerConeAngle = std::cos(Deg2Rad(sl.InnerConeAngle));
					ld.OuterConeAngle = std::cos(Deg2Rad(sl.OuterConeAngle));
					activeLights.push_back(ld);
				}
			);

			renderer.SetGlobalLightData(activeLights, env);

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

