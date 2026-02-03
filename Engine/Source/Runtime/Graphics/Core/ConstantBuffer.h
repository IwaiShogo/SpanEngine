#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	// 256バイトアライメント計算用ヘルパー
	inline uint32 CalcConstantBufferByteSize(uint32 byteSize)
	{
		return (byteSize + 255) & ~255;
	}

	template <typename T>
	class ConstantBuffer
	{
	public:
		// 初期化
		bool Initialize(ID3D12Device* device)
		{
			uint32 sizeInBytes = CalcConstantBufferByteSize(sizeof(T));

			// アップロードヒープを作成 (CPUから書き込む用)
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Alignment = 0;
			resourceDesc.Width = sizeInBytes;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			if (FAILED(device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&resource))))
			{
				SPAN_ERROR("Failed to create Constant Buffer!");
				return false;
			}

			// マップしてポインタを取得しておく (毎回Map/Unmapしなくて良い)
			D3D12_RANGE readRange = { 0, 0 };
			if (FAILED(resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData))))
			{
				SPAN_ERROR("Failed to map Constant Buffer!");
				return false;
			}

			return true;
		}

		void Shutdown()
		{
			if (resource)
			{
				resource->Unmap(0, nullptr);
				mappedData = nullptr;
				resource.Reset();
			}
		}

		// データをGPUメモリに書き込む
		void Update(const T& data)
		{
			if (mappedData)
			{
				memcpy(mappedData, &data, sizeof(T));
			}
		}

		// GPU上のアドレスを取得
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
		{
			return resource->GetGPUVirtualAddress();
		}

	private:
		ComPtr<ID3D12Resource> resource;
		T* mappedData = nullptr;
	};
}

