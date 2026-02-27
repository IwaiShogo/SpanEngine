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

		// コンピュートシェーダーの初期化を追加
		if (!InitializeCompute(device)) return false;

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

		// 4. Frustums Buffer
		m_frustumsBuffer = std::make_unique<ComputeBuffer>();
		m_frustumsBuffer->Initialize(device, sizeof(float) * 16, numTiles, true);

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

	void LightManager::ExecuteLightCulling(ID3D12GraphicsCommandList* cmd, const Matrix4x4& projectionMatrix, uint32 screenWidth, uint32 screenHeight)
	{
		if (!cmd || !m_frustumsBuffer || screenWidth == 0 || screenHeight == 0) return;

		// 1. パイプラインとルートシグネチャのセット
		cmd->SetPipelineState(m_psoFrustums.Get());
		cmd->SetComputeRootSignature(m_computeRootSignature.Get());

		// 2. 定数の準備とセット
		uint32 numTilesX = (screenWidth + TILE_SIZE - 1) / TILE_SIZE;
		uint32 numTilesY = (screenHeight + TILE_SIZE - 1) / TILE_SIZE;

		DirectX::XMVECTOR det;
		DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&det, projectionMatrix.ToXM());
		Matrix4x4 invProjMat;
		invProjMat.FromXM(XMMatrixTranspose(invProj));

		struct CullingCB
		{
			Matrix4x4 InverseProjection;
			uint32 ScreenDimensions[2];
			uint32 NumTiles[2];
		} cbData;
		cbData.InverseProjection = invProjMat;
		cbData.ScreenDimensions[0] = screenWidth;
		cbData.ScreenDimensions[1] = screenHeight;
		cbData.NumTiles[0] = numTilesX;
		cbData.NumTiles[1] = numTilesY;

		cmd->SetComputeRoot32BitConstants(0, sizeof(CullingCB) / 4, &cbData, 0);

		// 3. UAVのセット
		// cmd->SetComputeRootDescriptorTable(1, m_frustumsBuffer->GetUAV());

		// 4. Dispatch
		uint32 dispatchX = (numTilesX + TILE_SIZE - 1) / TILE_SIZE;
		uint32 dispatchY = (numTilesY + TILE_SIZE - 1) / TILE_SIZE;

		// cmd->Dispatch(dispatchX, dispatchY, 1);
	}

	bool LightManager::InitializeCompute(ID3D12Device* device)
	{
		// 1. シェーダーのロード
		m_shaderFrustums = std::make_unique<Shader>();
		if (!m_shaderFrustums->Load(L"LightCulling.hlsl", ShaderType::Compute, "CS_ComputeFrustums")) return false;

		// 2. Root Signature の作成
		D3D12_ROOT_PARAMETER rootParams[2];

		// b0: CullingCB
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParams[0].Constants.ShaderRegister = 0;
		rootParams[0].Constants.RegisterSpace = 0;
		rootParams[0].Constants.Num32BitValues = 20;

		// u0: RWStructuredBuffer<Frustum>
		D3D12_DESCRIPTOR_RANGE uavRange = {};
		uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRange.NumDescriptors = 1;
		uavRange.BaseShaderRegister = 0;
		uavRange.OffsetInDescriptorsFromTableStart = 0;

		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &uavRange;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 2;
		rootSigDesc.pParameters = rootParams;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_computeRootSignature)))) return false;

		// 3. Pipeline State の作成
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_computeRootSignature.Get();
		psoDesc.CS = m_shaderFrustums->GetBytecode();

		if (FAILED(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_psoFrustums)))) return false;

		return true;
	}
}
