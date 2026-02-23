/*****************************************************************//**
 * @file	IBLBuilder.cpp
 * @brief	IBL用テクスチャの生成実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "IBLBuilder.h"

namespace Span
{
	bool IBLBuilder::Initialize(ID3D12Device* device)
	{
		// 1. Compute Shader のロード
		m_equirectToCubemapCS = new Shader();
		if (!m_equirectToCubemapCS->Load(L"EquirectangularToCubemap.hlsl", ShaderType::Compute, "CSMain"))
		{
			SPAN_ERROR("Failed to load EquirectangularToCubemap Compute Shader.");
			return false;
		}

		// 2. Compute Root Signature の作成
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 1;
		srvRange.BaseShaderRegister = 0;
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_DESCRIPTOR_RANGE uavRange = {};
		uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRange.NumDescriptors = 1;
		uavRange.BaseShaderRegister = 0;
		uavRange.RegisterSpace = 0;
		uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParameters[2];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRange;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &uavRange;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// サンプラー
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 1;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0; // s0
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = _countof(rootParameters);
		rootSigDesc.pParameters = rootParameters;
		rootSigDesc.NumStaticSamplers = 1;
		rootSigDesc.pStaticSamplers = &sampler;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)))
		{
			SPAN_ERROR("IBLBuilder RootSignature Serialize Failed: %s", (char*)error->GetBufferPointer());
			return false;
		}

		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_computeRootSignature))))
			return false;

		// 3. Compute Pipeline State Object (PSO) の作成
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_computeRootSignature.Get();
		psoDesc.CS = m_equirectToCubemapCS->GetBytecode();

		if (FAILED(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_equirectToCubemapPSO))))
		{
			SPAN_ERROR("Failed to create IBLBuilder Compute PSO.");
			return false;
		}

		SPAN_LOG("IBLBuilder Initialized Successfully.");
		return true;
	}

	void IBLBuilder::Shutdown()
	{
		SAFE_DELETE(m_equirectToCubemapCS);
		m_equirectToCubemapPSO.Reset();
		m_computeRootSignature.Reset();
	}

	void IBLBuilder::GenerateCubemapFromPanorama(ID3D12Device* device, ID3D12GraphicsCommandList* cmd, D3D12_CPU_DESCRIPTOR_HANDLE panoramaSRV, Texture* outCubemap, uint32 cubemapSize)
	{
		if (!device || !cmd || !outCubemap) return;

		// 1. 一時的なディスクリプタヒープを2枠作成
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 2;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		ComPtr<ID3D12DescriptorHeap> tempHeap;
		device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&tempHeap));

		uint32 descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CPU_DESCRIPTOR_HANDLE destHandle = tempHeap->GetCPUDescriptorHandleForHeapStart();

		// 2. カタログにテクスチャの情報をコピー
		// [0] 入力パノラマ画像をコピー
		device->CopyDescriptorsSimple(1, destHandle, panoramaSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// [1] 出力Cubemapをコピー
		destHandle.ptr += descriptorSize;
		device->CopyDescriptorsSimple(1, destHandle, outCubemap->GetUAVCPU(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// 3. この一時カタログをGPUにセット
		ID3D12DescriptorHeap* heaps[] = { tempHeap.Get() };
		cmd->SetDescriptorHeaps(1, heaps);

		cmd->SetPipelineState(m_equirectToCubemapPSO.Get());
		cmd->SetComputeRootSignature(m_computeRootSignature.Get());

		// 4. スロットにカタログの住所を教える
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = tempHeap->GetGPUDescriptorHandleForHeapStart();
		cmd->SetComputeRootDescriptorTable(0, gpuHandle);	// t0: パノラマ
		gpuHandle.ptr += descriptorSize;
		cmd->SetComputeRootDescriptorTable(1, gpuHandle);	// u0: Cubemap

		// 5. Compute Shader を起動
		uint32 threadGroupsX = cubemapSize / 8;
		uint32 threadGroupsY = cubemapSize / 8;
		uint32 threadGroupsZ = 6;

		cmd->Dispatch(threadGroupsX, threadGroupsY, threadGroupsZ);
	}
}
