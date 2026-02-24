#include "Texture.h"

// 画像読み込みライブラリの実装を定義
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Span
{
	void Texture::Shutdown()
	{
		resource.Reset();
		uploadBuffer.Reset();
		srvHeap.Reset();
		uavHeap.Reset();
	}

	bool Texture::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const std::string& filepath)
	{
		SPAN_LOG("Loading Texture: %s", filepath.c_str());
		m_FilePath = filepath;

		// 拡張子でHDRかどうかを判定
		m_IsHDR = (filepath.length() > 4 && filepath.substr(filepath.length() - 4) == ".hdr");

		int w, h, channels;
		void* data = nullptr;
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		uint64_t bytesPerPixel = 4;

		if (m_IsHDR)
		{
			// HDR画像は float の配列として読み込む
			data = stbi_loadf(filepath.c_str(), &w, &h, &channels, 4);
			format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			bytesPerPixel = 16;
		}
		else
		{
			// LDR画像は unsigned char 配列として読み込む
			data = stbi_load(filepath.c_str(), &w, &h, &channels, 4);
		}

		if (!data)
		{
			SPAN_ERROR("Failed to load image: %s", filepath.c_str());
			return false;
		}

		width = static_cast<uint32_t>(w);
		height = static_cast<uint32_t>(h);

		// 2. GPUへアップロード
		if (!UploadTexture(device, commandQueue, data, width, height, bytesPerPixel, format))
		{
			stbi_image_free(data);
			return false;
		}

		// メモリ解放
		stbi_image_free(data);

		// 3. SRV (Shader Resource View) ヒープの作成
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap))))
		{
			SPAN_ERROR("Failed to create SRV Heap");
			return false;
		}

		// 4. SRV (ビュー) の作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		srvHandleCPU = srvHeap->GetCPUDescriptorHandleForHeapStart();
		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandleCPU);

		return true;
	}

	bool Texture::InitializeAsCubemap(ID3D12Device* device, uint32_t size, uint32_t mipLevels)
	{
		width = size;
		height = size;
		m_IsHDR = true;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = size;
		desc.Height = size;
		desc.DepthOrArraySize = 6;
		desc.MipLevels = mipLevels;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };
		if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource)))) return false;

		// UAV (書き込み用) ヒープとビューの作成
		D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
		uavHeapDesc.NumDescriptors = 1;
		uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&uavHeap));

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = desc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice = 0;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.ArraySize = 6;
		device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, uavHeap->GetCPUDescriptorHandleForHeapStart());

		// SRV (読み込み用) ヒープとビューの作成
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mipLevels;
		srvHandleCPU = srvHeap->GetCPUDescriptorHandleForHeapStart();
		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandleCPU);

		return true;
	}

	bool Texture::InitializeAsTexture2D(ID3D12Device* device, uint32_t w, uint32_t h, DXGI_FORMAT format)
	{
		width = w;
		height = h;
		m_IsHDR = true;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = w;
		desc.Height = h;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };
		if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource)))) return false;

		// UAV (書き込み用)
		D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
		uavHeapDesc.NumDescriptors = 1;
		uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&uavHeap));

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = desc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, uavHeap->GetCPUDescriptorHandleForHeapStart());

		// SRV (読み込み用)
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvHandleCPU = srvHeap->GetCPUDescriptorHandleForHeapStart();
		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandleCPU);

		return true;
	}

	bool Texture::UploadTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
		const void* initialData, uint64_t w, uint64_t h, uint64_t bytesPerPixel, DXGI_FORMAT format)
	{
		// リソース記述
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = format;
		textureDesc.Width = w;
		textureDesc.Height = h;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		// 1. デフォルトヒープ（VRAM）にテクスチャリソースを作成
		D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };

		if (FAILED(device->CreateCommittedResource(
			&defaultHeap,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&resource))))
		{
			return false;
		}

		// 2. アップロード用バッファのサイズ計算
		uint64_t uploadBufferSize = 0;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		UINT64 rowSizeInBytes;
		UINT numRows;
		device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &uploadBufferSize);

		// 3. アップロードヒープ（CPU書き込み可）のリソース作成
		D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = uploadBufferSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (FAILED(device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer))))
		{
			return false;
		}

		// 4. データをアップロードバッファにコピー
		void* mappedData = nullptr;
		uploadBuffer->Map(0, nullptr, &mappedData);

		const uint8_t* srcData = reinterpret_cast<const uint8_t*>(initialData);
		uint8_t* destData = reinterpret_cast<uint8_t*>(mappedData);

		for (UINT i = 0; i < h; ++i)
		{
			memcpy(destData + (i * footprint.Footprint.RowPitch),
				srcData + (i * w * bytesPerPixel),
				w * bytesPerPixel);
		}
		uploadBuffer->Unmap(0, nullptr);

		// 5. コマンドリストを作ってコピー命令を発行
		ComPtr<ID3D12CommandAllocator> cmdAlloc;
		ComPtr<ID3D12GraphicsCommandList> cmdList;
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));

		D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
		srcLoc.pResource = uploadBuffer.Get();
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLoc.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
		dstLoc.pResource = resource.Get();
		dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLoc.SubresourceIndex = 0;

		cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

		// リソースバリア (コピー先 -> シェーダーリソース読み取り)
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		cmdList->ResourceBarrier(1, &barrier);

		cmdList->Close();

		// 実行して待機
		ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
		commandQueue->ExecuteCommandLists(1, ppCommandLists);

		// フェンス同期
		ComPtr<ID3D12Fence> fence;
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		commandQueue->Signal(fence.Get(), 1);

		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fence->GetCompletedValue() < 1)
		{
			fence->SetEventOnCompletion(1, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		CloseHandle(fenceEvent);

		return true;
	}
}
