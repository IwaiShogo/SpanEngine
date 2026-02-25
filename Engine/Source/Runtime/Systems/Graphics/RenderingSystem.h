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

			// HDRIの動的ロード監視
			if (env.Mode == SkyboxMode::HDRI && !env.HDRIPath.empty())
			{
				renderer.LoadEnvironmentMap(env.HDRIPath);
			}

			// 1. Light Collection & Shadow Matrix Collection
			// ============================================================
			std::vector<LightDataGPU> activeLights;

			// --- Directional Light ---
			Matrix4x4 dirLightMatrix = Matrix4x4::Identity();
			bool dirLightCastShadow = false;

			world->ForEach<DirectionalLight, LocalToWorld>(
				[&](Entity, DirectionalLight& dl, LocalToWorld& ltw)
				{
					LightDataGPU ld = {};
					ld.Type = 0;
					ld.Direction = Vector3::Normalize(Vector3(ltw.Value.m[2][0], ltw.Value.m[2][1], ltw.Value.m[2][2]));
					ld.Color = dl.Color; ld.Intensity = dl.Intensity; ld.CastShadows = dl.CastShadows ? 1 : 0;

					// 行列計算
					Vector3 lightTarget = renderer.GetCameraPosition();
					Vector3 lightPos = lightTarget - ld.Direction * (dl.ShadowMaxDistance * 0.5f);
					Vector3 up = (std::abs(Vector3::Dot(ld.Direction, Vector3::Up)) > 0.999f) ? Vector3::Forward : Vector3::Up;
					dirLightMatrix = Matrix4x4::LookAtLH(lightPos, lightTarget, up) * Matrix4x4::OrthographicLH(dl.ShadowAreaSize, dl.ShadowAreaSize, 1.0f, dl.ShadowMaxDistance);

					ld.ShadowMatrix = dirLightMatrix;
					activeLights.push_back(ld);
					dirLightCastShadow = dl.CastShadows;
				}
			);

			// --- Spot Light ---
			int spotShadowCount = 0;

			world->ForEach<SpotLight, LocalToWorld>(
				[&](Entity, SpotLight& sl, LocalToWorld& ltw)
				{
					LightDataGPU ld = {};
					ld.Type = 2;
					ld.Position = Vector3(ltw.Value.m[3][0], ltw.Value.m[3][1], ltw.Value.m[3][2]);
					ld.Direction = Vector3::Normalize(Vector3(ltw.Value.m[2][0], ltw.Value.m[2][1], ltw.Value.m[2][2]));
					ld.Color = sl.Color; ld.Intensity = sl.Intensity; ld.Range = sl.Range;
					ld.InnerConeAngle = std::cos(Deg2Rad(sl.InnerConeAngle)); ld.OuterConeAngle = std::cos(Deg2Rad(sl.OuterConeAngle));
					ld.CastShadows = sl.CastShadows ? 1 : 0;
					ld.ShadowIndex = -1;

					// 影を落とす設定で、かつ最大数(4)未満なら行列を計算
					if (sl.CastShadows && spotShadowCount < 4)
					{
						ld.ShadowIndex = spotShadowCount;
						Vector3 up = (std::abs(Vector3::Dot(ld.Direction, Vector3::Up)) > 0.999f) ? Vector3::Forward : Vector3::Up;
						Matrix4x4 view = Matrix4x4::LookAtLH(ld.Position, ld.Position + ld.Direction, up);
						// 透視投影(Perspective)を使用！ Fov は OuterCone の2倍の広さ
						Matrix4x4 proj = Matrix4x4::PerspectiveFovLH(Deg2Rad(sl.OuterConeAngle * 2.0f), 1.0f, 0.1f, sl.Range);
						ld.ShadowMatrix = view * proj;
						spotShadowCount++;
					}
					else
					{
						ld.ShadowMatrix = Matrix4x4::Identity().Transpose();
					}

					activeLights.push_back(ld);
				}
			);

			// --- Point Light ---
			int pointShadowCount = 0;

			world->ForEach<PointLight, LocalToWorld>(
				[&](Entity, PointLight& pl, LocalToWorld& ltw)
				{
					LightDataGPU ld = {};
					ld.Type = 1;
					ld.Position = Vector3(ltw.Value.m[3][0], ltw.Value.m[3][1], ltw.Value.m[3][2]);
					ld.Color = pl.Color;
					ld.Intensity = pl.Intensity;
					ld.Range = pl.Range;
					ld.CastShadows = pl.CastShadows ? 1 : 0;
					ld.ShadowIndex = -1;

					if (pl.CastShadows && pointShadowCount < 1)
					{
						ld.ShadowIndex = pointShadowCount;
						pointShadowCount++;
					}
					activeLights.push_back(ld);
				}
			);

			renderer.SetGlobalLightData(activeLights, env);

			// ============================================================
			// 2. Shadow Passes
			// ============================================================

			// --- Directional Shadow Pass ---
			if (auto dirPass = renderer.GetDirShadowPass())
			{
				dirPass->BeginPass(cmd);
				dirPass->SetRenderTarget(cmd, 0);
				if (dirLightCastShadow) {
					world->ForEach<MeshFilter, MeshRenderer, LocalToWorld>([&](Entity, MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw) {
						if (mf.mesh && mr.material && mr.CastShadows) dirPass->DrawMesh(&renderer, cmd, mf.mesh, ltw.Value, dirLightMatrix);
						});
				}
				dirPass->EndPass(cmd);
			}

			// --- Spot Shadow Pass (Array) ---
			if (auto spotPass = renderer.GetSpotShadowPass())
			{
				spotPass->BeginPass(cmd);
				for (int i = 0; i < spotShadowCount; ++i)
				{
					spotPass->SetRenderTarget(cmd, i);

					// このスライス用の行列を検索
					Matrix4x4 spotMatrix = Matrix4x4::Identity();
					for (auto& l : activeLights)
					{
						if (l.Type == 2 && l.ShadowIndex == i)
						{
							spotMatrix = l.ShadowMatrix;
							break;
						}
					}

					world->ForEach<MeshFilter, MeshRenderer, LocalToWorld>([&](Entity, MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw) {
						if (mf.mesh && mr.material && mr.CastShadows) spotPass->DrawMesh(&renderer, cmd, mf.mesh, ltw.Value, spotMatrix);
						});
				}
				spotPass->EndPass(cmd);
			}

			// --- Point Shadow Pass ---
			if (auto pointPass = renderer.GetPointShadowPass())
			{
				pointPass->BeginPass(cmd);
				for (auto& l : activeLights)
				{
					if (l.Type == 1 && l.ShadowIndex == 0)
					{
						Vector3 pos = l.Position;
						float range = l.Range;

						// 6方向のカメラ設定
						Matrix4x4 views[6] = {
							Matrix4x4::LookAtLH(pos, pos + Vector3::Right,		Vector3::Up),		// +X
							Matrix4x4::LookAtLH(pos, pos + Vector3::Left,		Vector3::Up),		// -X
							Matrix4x4::LookAtLH(pos, pos + Vector3::Up,			Vector3::Back),		// +Y
							Matrix4x4::LookAtLH(pos, pos + Vector3::Down,		Vector3::Forward),	// -Y
							Matrix4x4::LookAtLH(pos, pos + Vector3::Forward,	Vector3::Up),		// +Z
							Matrix4x4::LookAtLH(pos, pos + Vector3::Back,		Vector3::Up)		// -Z
						};

						// 角度90度(PI/2)の透視投影
						Matrix4x4 proj = Matrix4x4::PerspectiveFovLH(HalfPI, 1.0f, 0.1f, range);

						// 6面全て描画
						for (int face = 0; face < 6; ++face)
						{
							pointPass->SetRenderTarget(cmd, face);
							Matrix4x4 cubeMatrix = views[face] * proj;

							world->ForEach<MeshFilter, MeshRenderer, LocalToWorld>([&](Entity, MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw) {
								if (mf.mesh && mr.material && mr.CastShadows)
									pointPass->DrawMesh(&renderer, cmd, mf.mesh, ltw.Value, cubeMatrix);
							});
						}
					}
				}
				pointPass->EndPass(cmd);
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

			// 3. Main Pass (Opaque -> Skybox -> Grid -> Capture -> Glass -> Transparent)
			// ============================================================
			// [1] Opaque
			world->ForEach<MeshFilter, MeshRenderer, LocalToWorld>(
				[&](Entity, MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw)
				{
					if (!mf.mesh || !mr.material) return;

					if (mr.material->GetBlendMode() != BlendMode::Transparent && mr.material->GetData().Transmission <= 0.0f)
					{
						renderer.DrawMesh(mf.mesh, mr.material, ltw.Value);
					}
				}
			);

			// Skybox
			if (auto skyboxPass = renderer.GetSkyboxPass())
			{
				skyboxPass->Render(&renderer, cmd, env, renderer.GetViewMatrix(), renderer.GetProjectionMatrix(), renderer.GetCameraPosition(), renderer.GetEnvironmentCubemap());
			}

			// Editor Pass (Grid)
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

			// Opaque Capture
			auto opaqueTex = renderer.GetOpaqueCaptureTexture();
			if (opaqueTex && opaqueTex->GetResource() && sceneBuffer.GetResource())
			{
				D3D12_RESOURCE_BARRIER barriers[2] = {};

				// SceneBuffer を RENDER_TARGET から COPY_SOURCE へ遷移
				barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barriers[0].Transition.pResource = sceneBuffer.GetResource();
				barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
				barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				// OpaqueTex を PIXEL_SHADER_RESOURCE から COPY_DEST へ遷移
				barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barriers[1].Transition.pResource = opaqueTex->GetResource();
				barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
				barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				cmd->ResourceBarrier(2, barriers);

				// GPU上で高速に画像コピー
				cmd->CopyResource(opaqueTex->GetResource(), sceneBuffer.GetResource());

				// 状態を元に戻す
				barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
				barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

				barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

				cmd->ResourceBarrier(2, barriers);
			}

			// [2] ガラス (Opaque だが Transmission が 0 より大きいもの
			world->ForEach<MeshFilter, MeshRenderer, LocalToWorld>(
				[&](Entity, MeshFilter& mf, MeshRenderer& mr, LocalToWorld& ltw)
				{
					if (!mf.mesh || !mr.material) return;

					if (mr.material->GetBlendMode() != BlendMode::Transparent && mr.material->GetData().Transmission > 0.0f)
					{
						renderer.DrawMesh(mf.mesh, mr.material, ltw.Value);
					}
				}
			);

			// [3] Transparent
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

