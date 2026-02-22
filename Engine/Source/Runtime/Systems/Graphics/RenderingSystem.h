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

// Render Passes
#include "Graphics/Passes/ShadowPass.h"
#include "Graphics/Passes/GridPass.h"
#include "Graphics/Passes/SkyboxPass.h"

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
			ID3D12GraphicsCommandList* cmd = renderer.GetCommandList();
			auto world = GetWorld();

			EnvironmentSettings& env = Application::Get().GetActiveScene().Environment;

			// 1. Light Collection & Shadow Matrix Collection
			// ============================================================
			std::vector<LightDataGPU> activeLights;
			Vector3 mainLightDir = Vector3::Down;
			bool mainLightCastShadows = true;

			float shadowArea = 50.0f;
			float shadowDist = 200.0f;

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
					mainLightDir = ld.Direction;
					mainLightCastShadows = dl.CastShadows;
					shadowArea = dl.ShadowAreaSize;
					shadowDist = dl.ShadowMaxDistance;
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

			// 1. 太陽のカメラを、常に「現在のプレイヤー(カメラ)の位置」に追従させる
			Vector3 camPos = renderer.GetCameraPosition();
			Vector3 lightTarget = camPos;

			// 2. ターゲットから光の逆方向へ十分な距離を離してカメラを置く
			Vector3 lightPos = lightTarget - mainLightDir * (shadowDist * 0.5f);

			// 3. 上方向ベクトルの計算
			Vector3 up = Vector3::Up;
			if (std::abs(Vector3::Dot(mainLightDir, Vector3::Up)) > 0.999f) up = Vector3::Forward;

			Matrix4x4 lightView = Matrix4x4::LookAtLH(lightPos, lightTarget, up);

			// 4. 投影範囲
			Matrix4x4 lightProj = Matrix4x4::OrthographicLH(shadowArea, shadowArea, 1.0f, shadowDist);

			Matrix4x4 lightSpaceMatrix = lightView * lightProj;

			if (auto shadowPass = renderer.GetShadowPass())
			{
				shadowPass->SetLightSpaceMatrix(lightSpaceMatrix);
			}

			// ライトデータをメインパス用に転送
			renderer.SetGlobalLightData(activeLights, env);

			// 2. Shadow Pass
			// ============================================================
			if (auto shadowPass = renderer.GetShadowPass())
			{
				if (mainLightCastShadows)
				{
					shadowPass->BeginPass(cmd);

					world->ForEach<MeshFilter, MeshRenderer, LocalToWorld>(
						[&](Entity, MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw)
						{
							if (mf.mesh && mr.material && mr.material->GetBlendMode() != BlendMode::Transparent)
							{
								if (mr.CastShadows)
								{
									shadowPass->DrawMesh(&renderer, cmd, mf.mesh, ltw.Value);
								}
							}
						}
					);

					shadowPass->EndPass(cmd);
				}
				else
				{
					shadowPass->BeginPass(cmd);
					shadowPass->EndPass(cmd);
				}
			}

			// メインレンダーターゲットの復元
			auto& sceneBuffer = Application::Get().GetSceneBuffer();

			// 描画先をSceneBufferに戻す
			D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneBuffer.GetRTV();
			D3D12_CPU_DESCRIPTOR_HANDLE dsv = sceneBuffer.GetDSV();
			cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

			// 描画範囲をSceneBufferのサイズに戻す
			D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)sceneBuffer.GetWidth(), (float)sceneBuffer.GetHeight(), 0.0f, 1.0f };
			D3D12_RECT scissor = { 0, 0, (long)sceneBuffer.GetWidth(), (long)sceneBuffer.GetHeight() };
			cmd->RSSetViewports(1, &vp);
			cmd->RSSetScissorRects(1, &scissor);

			// 3. Main Pass (Opaque -> Skybox -> Transparent)
			// ============================================================
			// Opaque
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

			// Skybox
			if (auto skyboxPass = renderer.GetSkyboxPass())
			{
				skyboxPass->Render(&renderer, cmd, env, renderer.GetViewMatrix(), renderer.GetProjectionMatrix(), renderer.GetCameraPosition());
			}

			// Transparent
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

			// 4. Editor Pass (Grid)
			// ============================================================
			if (auto gridPass = renderer.GetGridPass())
			{
				struct SceneCB { Matrix4x4 view; Matrix4x4 proj; Vector3 camPos; float pad; };
				SceneCB sceneData = {
					renderer.GetViewMatrix().Transpose(),
					renderer.GetProjectionMatrix().Transpose(),
					renderer.GetCameraPosition(), 0.0f
				};

				D3D12_GPU_VIRTUAL_ADDRESS cbAddr = renderer.AllocateCBV(&sceneData, sizeof(SceneCB));
				if (cbAddr != 0)
				{
					gridPass->Render(cmd, cbAddr);
				}
			}
		}
	};
}

