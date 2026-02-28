/*****************************************************************//**
 * @file	ComputeBuffer.cpp
 * @brief	ComputeBufferの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "ComputeBuffer.h"

namespace Span
{
	bool ComputeBuffer::Initialize(ID3D12Device* device, uint32 elementSize, uint32 elementCount, bool isUAV)
	{
		m_elementSize = elementSize;
		m_elementCount = elementCount;
		m_isUAV = isUAV;

		uint64 bufferSize = (uint64)elementSize * elementCount;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = isUAV ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = bufferSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = isUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

		if (FAILED(device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
			isUAV ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&m_resource))))
		{
			return false;
		}

		// Upload Heapの場合はメモリをマップしっぱなしにする
		if (!isUAV)
		{
			D3D12_RANGE readRange = { 0, 0 };
			m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedData));
		}

		// SRV の作成
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = elementCount;
		srvDesc.Buffer.StructureByteStride = elementSize;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());

		// UAV の作成
		if (isUAV)
		{
			D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
			uavHeapDesc.NumDescriptors = 1;
			uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&m_uavHeap));

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = elementCount;
			uavDesc.Buffer.StructureByteStride = elementSize;
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			device->CreateUnorderedAccessView(m_resource.Get(), nullptr, &uavDesc, m_uavHeap->GetCPUDescriptorHandleForHeapStart());
		}

		return true;
	}

	void ComputeBuffer::Shutdown()
	{
		if (!m_isUAV && m_resource && m_mappedData)
		{
			m_resource->Unmap(0, nullptr);
			m_mappedData = nullptr;
		}

		// ヒープ
		m_srvHeap.Reset();
		m_uavHeap.Reset();

		m_resource.Reset();
	}

	void ComputeBuffer::UpdateData(const void* data, size_t dataSize)
	{
		if (!m_isUAV && m_mappedData && data)
		{
			memcpy(m_mappedData, data, dataSize);
		}
	}
}
