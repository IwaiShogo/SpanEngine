/*****************************************************************//**
 * @file	ShadowMap.cpp
 * @brief	ShadowMapの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/


#include "ShadowMap.h"

namespace Span
{
	bool ShadowMap::Initialize(ID3D12Device* device, uint32 width, uint32 height, uint32 arraySize, bool isCube)
	{
		m_width = width;
		m_height = height;
		m_arraySize = arraySize;
		m_dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		// 1. DSVヒープの作成
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = arraySize;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)))) return false;

		// 2. SRVヒープの作成
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダーからアクセス可能にする
		if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)))) return false;

		// 3. テクスチャリソースの作成
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.DepthOrArraySize = arraySize;
		texDesc.MipLevels = 1;
		texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear = {};
		optClear.Format = DXGI_FORMAT_D32_FLOAT;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		if (FAILED(device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE,
			&texDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&optClear, IID_PPV_ARGS(&m_resource))))
		{
			return false;
		}

		// 4. DSV (書き込み用) の作成
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		for (uint32 i = 0; i < arraySize; ++i)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;

			if (arraySize > 1)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				dsvDesc.Texture2DArray.ArraySize = 1;
				dsvDesc.Texture2DArray.MipSlice = 0;
			}
			else
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;
			}

			device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, dsvHandle);
			dsvHandle.ptr += m_dsvDescriptorSize;
		}

		// 5. SRV (読み込み用) の作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

		if (isCube && arraySize == 6)	// Point Light
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = 1;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
		else if (arraySize > 1)	// Spot Light
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = arraySize;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
		}
		else // Dir Light
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
		}

		device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());

		return true;
	}

	void ShadowMap::Shutdown()
	{
		m_resource.Reset();
		m_srvHeap.Reset();
		m_dsvHeap.Reset();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDSV(uint32 sliceIndex) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<SIZE_T>(sliceIndex) * m_dsvDescriptorSize;
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetSRV() const
	{
		return m_srvHeap->GetGPUDescriptorHandleForHeapStart();
	}
}
