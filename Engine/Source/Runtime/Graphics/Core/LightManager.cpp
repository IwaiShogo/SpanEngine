/*****************************************************************//**
 * @file	LightManager.cpp
 * @brief	LightManagerの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "LightManager.h"
#include "Runtime/Scene/EnvironmentSettings.h"

namespace Span
{
	bool LightManager::Initialize(ID3D12Device* device)
	{
		m_lightConstantBuffer = std::make_unique<ConstantBuffer<GlobalLightData>>();
		if (!m_lightConstantBuffer->Initialize(device)) return false;

		return true;
	}

	void LightManager::Shutdown()
	{
		m_lightConstantBuffer.reset();
	}

	void LightManager::OnResize(ID3D12Device* device, uint32 width, uint32 height)
	{
		if (!device || width == 0 || height == 0) return;

		// タイル数の計算
		uint32 numTilesX = (width + TILE_SIZE - 1) / TILE_SIZE;
		uint32 numTilesY = (height + TILE_SIZE - 1) / TILE_SIZE;
		uint32 numTiles = numTilesX * numTilesY;

		// バッファを解放して再作成
		m_lightIndexCounter.reset();
		m_lightIndexList.reset();
		m_lightGrid.reset();

		// 1. Opaque Light Index Counter
		m_lightIndexCounter = std::make_unique<ComputeBuffer>();
		m_lightIndexCounter->Initialize(device, sizeof(uint32), 1, true);

		// 2. Opaque Light Index List
		m_lightIndexList = std::make_unique<ComputeBuffer>();
		m_lightIndexList->Initialize(device, sizeof(uint32), numTiles * MAX_LIGHTS_PER_TILE, true);

		// 3. Light Grid
		m_lightGrid = std::make_unique<ComputeBuffer>();
		m_lightGrid->Initialize(device, sizeof(uint32) * 2, numTiles, true);

		SPAN_LOG("LightManager Resized: %dx%d (%d Tiles)", width, height, numTiles);
	}

	void LightManager::UpdateLightData(const std::vector<LightDataGPU>& lights, const EnvironmentSettings& env, const Vector3& cameraPos, bool enableSSAO)
	{
		m_currentLightData.CameraPosition = cameraPos;
		m_currentLightData.Exposure = env.Exposure;
		m_currentLightData.AmbientIntensity = env.AmbientIntensity;
		m_currentLightData.EnvReflectionIntensity = env.EnvReflectionIntensity;

		// IBLカラーをLiner
		m_currentLightData.SkyTopColor = Vector3(pow(env.SkyTopColor.x, 2.2f), pow(env.SkyTopColor.y, 2.2f), pow(env.SkyTopColor.z, 2.2f));
		m_currentLightData.SkyHorizonColor = Vector3(pow(env.SkyHorizonColor.x, 2.2f), pow(env.SkyHorizonColor.y, 2.2f), pow(env.SkyHorizonColor.z, 2.2f));
		m_currentLightData.SkyBottomColor = Vector3(pow(env.SkyBottomColor.x, 2.2f), pow(env.SkyBottomColor.y, 2.2f), pow(env.SkyBottomColor.z, 2.2f));

		m_currentLightData.SkyMode = (env.Mode == SkyboxMode::HDRI) ? 1 : 0;
		m_currentLightData.EnableSSAO = enableSSAO ? 1 : 0;

		m_currentLightData.DirectionalLightSpaceMatrix = Matrix4x4::Identity().Transpose();

		m_currentLightData.ActiveLightCount = std::min((int)lights.size(), MAX_LIGHTS);
		for (int i = 0; i < m_currentLightData.ActiveLightCount; ++i)
		{
			m_currentLightData.Lights[i] = lights[i];

			// ShadowMatrix の設定
			if (lights[i].Type == 0)
			{
				m_currentLightData.DirectionalLightSpaceMatrix = lights[i].ShadowMatrix.Transpose();
			}
		}

		if (m_lightConstantBuffer)
		{
			m_lightConstantBuffer->Update(m_currentLightData);
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS LightManager::GetLightBufferAddress() const
	{
		return m_lightConstantBuffer ? m_lightConstantBuffer->GetGPUVirtualAddress() : 0;
	}
}
