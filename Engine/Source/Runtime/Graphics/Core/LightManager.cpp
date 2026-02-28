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

		// ライト配列
		m_lightDataBuffer = std::make_unique<ComputeBuffer>();
		if (!m_lightDataBuffer->Initialize(device, sizeof(LightDataGPU), MAX_LIGHTS, false)) return false;

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

		// ComputeShader用のDescriptorHeapを作成し、バッファ群をコピー
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 5;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_computeDescriptorHeap));

		uint32 increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CPU_DESCRIPTOR_HANDLE destCpu = m_computeDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		device->CopyDescriptorsSimple(1, destCpu, m_lightDataBuffer->GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); destCpu.ptr += increment;
		device->CopyDescriptorsSimple(1, destCpu, m_frustumsBuffer->GetUAV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); destCpu.ptr += increment;
		device->CopyDescriptorsSimple(1, destCpu, m_lightIndexCounter->GetUAV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); destCpu.ptr += increment;
		device->CopyDescriptorsSimple(1, destCpu, m_lightIndexList->GetUAV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); destCpu.ptr += increment;
		device->CopyDescriptorsSimple(1, destCpu, m_lightGrid->GetUAV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SPAN_LOG("LightManager Resized: %dx%d (%d Tiles)", width, height, numTiles);
	}

	void LightManager::UpdateLightData(const std::vector<LightDataGPU>& lights, const EnvironmentSettings& env, const Vector3& cameraPos, bool enableSSAO, uint32 screenWidth, uint32 screenHeight)
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
		m_currentLightData.ScreenWidth = screenWidth;
		m_currentLightData.ScreenHeight = screenHeight;

		m_currentLightData.DirectionalLightSpaceMatrix = Matrix4x4::Identity().Transpose();
		m_currentLightData.ActiveLightCount = std::min((int)lights.size(), MAX_LIGHTS);

		// CPU上で配列を作って一気に転送
		std::vector<LightDataGPU> lightDataArray;
		lightDataArray.reserve(m_currentLightData.ActiveLightCount);

		for (int i = 0; i < m_currentLightData.ActiveLightCount; ++i)
		{
			lightDataArray.push_back(lights[i]);
			if (lights[i].Type == 0) m_currentLightData.DirectionalLightSpaceMatrix = lights[i].ShadowMatrix.Transpose();
		}

		if (m_lightConstantBuffer)
		{
			m_lightConstantBuffer->Update(m_currentLightData);
		}

		if (m_lightDataBuffer && !lightDataArray.empty())
		{
			m_lightDataBuffer->UpdateData(lightDataArray.data(), lightDataArray.size() * sizeof(LightDataGPU));
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS LightManager::GetLightBufferAddress() const
	{
		return m_lightConstantBuffer ? m_lightConstantBuffer->GetGPUVirtualAddress() : 0;
	}

	void LightManager::ExecuteLightCulling(ID3D12Device* device, ID3D12GraphicsCommandList* cmd, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, uint32 screenWidth, uint32 screenHeight)
	{
		if (!device || !cmd || !m_frustumsBuffer || !m_computeDescriptorHeap || screenWidth == 0 || screenHeight == 0) return;

		// 定数の準備とセット
		uint32 numTilesX = (screenWidth + TILE_SIZE - 1) / TILE_SIZE;
		uint32 numTilesY = (screenHeight + TILE_SIZE - 1) / TILE_SIZE;

		DirectX::XMVECTOR det;
		DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&det, projectionMatrix.ToXM());
		Matrix4x4 invProjMat;
		invProjMat.FromXM(XMMatrixTranspose(invProj));

		Matrix4x4 viewMatTransposed;
		viewMatTransposed.FromXM(XMMatrixTranspose(viewMatrix.ToXM()));

		struct CullingCB
		{
			Matrix4x4 InvProjection;
			Matrix4x4 View;
			uint32 ScreenDim[2];
			uint32 NumTiles[2];
		} cbData;
		cbData.InvProjection = invProjMat;
		cbData.View = viewMatTransposed;
		cbData.ScreenDim[0] = screenWidth;
		cbData.ScreenDim[1] = screenHeight;
		cbData.NumTiles[0] = numTilesX;
		cbData.NumTiles[1] = numTilesY;

		ID3D12DescriptorHeap* heaps[] = { m_computeDescriptorHeap.Get() };
		cmd->SetDescriptorHeaps(1, heaps);

		// [0] Constants, [1] LightBufferCBV, [2] SRV Table(1個), [3] UAV Table(4個)
		cmd->SetComputeRootSignature(m_computeRootSignature.Get());
		cmd->SetComputeRoot32BitConstants(0, sizeof(CullingCB) / 4, &cbData, 0);
		cmd->SetComputeRootConstantBufferView(1, m_lightConstantBuffer->GetGPUVirtualAddress());
		cmd->SetComputeRootDescriptorTable(2, m_computeDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		D3D12_GPU_DESCRIPTOR_HANDLE uavStart = m_computeDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		uavStart.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // 1つ飛ばす(SRVの次)
		cmd->SetComputeRootDescriptorTable(3, uavStart);

		uint32 dispatchX = (numTilesX + TILE_SIZE - 1) / TILE_SIZE;
		uint32 dispatchY = (numTilesY + TILE_SIZE - 1) / TILE_SIZE;

		// 1. フラスタム計算パス
		cmd->SetPipelineState(m_psoFrustums.Get());
		cmd->Dispatch(dispatchX, dispatchY, 1);

		// 2. カウンタのリセットパス
		cmd->SetPipelineState(m_psoResetCounter.Get());
		cmd->Dispatch(1, 1, 1);

		// UAVバリア (リセットとフラスタム計算の完了を待つ)
		D3D12_RESOURCE_BARRIER barriers[2] = {};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV; barriers[0].UAV.pResource = m_frustumsBuffer->GetResource();
		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV; barriers[1].UAV.pResource = m_lightIndexCounter->GetResource();
		cmd->ResourceBarrier(2, barriers);

		// 3. ライトカリングパス
		cmd->SetPipelineState(m_psoCulling.Get());
		cmd->Dispatch(numTilesX, numTilesY, 1);
	}

	bool LightManager::InitializeCompute(ID3D12Device* device)
	{
		// 1. シェーダーのロード
		m_shaderFrustums = std::make_unique<Shader>();
		if (!m_shaderFrustums->Load(L"LightCulling.hlsl", ShaderType::Compute, "CS_ComputeFrustums")) return false;

		// カリングシェーダーのロード
		m_shaderCulling = std::make_unique<Shader>();
		if (!m_shaderCulling->Load(L"LightCulling.hlsl", ShaderType::Compute, "CS_LightCulling")) return false;

		// 2. Root Signature の作成
		D3D12_ROOT_PARAMETER rootParams[4];

		// b0: CullingCB (32bit Constants)
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParams[0].Constants.ShaderRegister = 0;
		rootParams[0].Constants.RegisterSpace = 0;
		rootParams[0].Constants.Num32BitValues = 36;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// b1: LightBuffer (CBV)
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParams[1].Descriptor.ShaderRegister = 1;
		rootParams[1].Descriptor.RegisterSpace = 0;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// t0: LightData Array (SRV)
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 1;
		srvRange.BaseShaderRegister = 0;
		srvRange.OffsetInDescriptorsFromTableStart = 0;

		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// u0 ~ u3: UAV Table
		D3D12_DESCRIPTOR_RANGE uavRange = {};
		uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRange.NumDescriptors = 4;
		uavRange.BaseShaderRegister = 0;
		uavRange.OffsetInDescriptorsFromTableStart = 0;

		rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[3].DescriptorTable.pDescriptorRanges = &uavRange;
		rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 4;
		rootSigDesc.pParameters = rootParams;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_computeRootSignature)))) return false;

		// 3. Pipeline State の作成
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_computeRootSignature.Get();

		// フラスタム計算用 PSO
		psoDesc.CS = m_shaderFrustums->GetBytecode();
		if (FAILED(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_psoFrustums)))) return false;

		m_shaderResetCounter = std::make_unique<Shader>();
		if (!m_shaderResetCounter->Load(L"LightCulling.hlsl", ShaderType::Compute, "CS_ResetCounter")) return false;

		psoDesc.CS = m_shaderResetCounter->GetBytecode();
		if (FAILED(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_psoResetCounter)))) return false;

		// カリング用シェーダーのロードと PSO の作成
		m_shaderCulling = std::make_unique<Shader>();
		if (!m_shaderCulling->Load(L"LightCulling.hlsl", ShaderType::Compute, "CS_LightCulling")) return false;

		psoDesc.CS = m_shaderCulling->GetBytecode();
		if (FAILED(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_psoCulling)))) return false;

		return true;
	}
}
