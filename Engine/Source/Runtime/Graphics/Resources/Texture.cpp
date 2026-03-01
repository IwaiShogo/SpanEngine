/*****************************************************************//**
 * @file	Texture.cpp
 * @brief	Textureの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "Texture.h"
#include <d3dx12.h>

namespace Span
{
	// std::string を std::wstring に変換するヘルパー
	static std::wstring ToWideString(const std::string& str)
	{
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

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

		// 拡張子の取得と小文字化
		std::string ext = std::filesystem::path(filepath).extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		// 拡張子でHDRかどうかを判定
		m_IsHDR = (ext == ".hdr");
		std::wstring wFilePath = ToWideString(filepath);

		DirectX::ScratchImage image;
		HRESULT hr = S_OK;

		// 1. 拡張子に応じたデコーダーで画像を読み込む
		if (ext == ".dds")
		{
			// DDSなら超高速ロード
			hr = DirectX::LoadFromDDSFile(wFilePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
		}
		else if (ext == ".hdr")
		{
			// HDR画像
			hr = DirectX::LoadFromHDRFile(wFilePath.c_str(), nullptr, image);
		}
		else
		{
			// PNG, JPG等の一般的なWIC対応フォーマット
			hr = DirectX::LoadFromWICFile(wFilePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
		}

		if (FAILED(hr))
		{
			SPAN_ERROR("Failed to load image via DirectXTex: %s (HRESULT: 0x%08X)", filepath.c_str(), hr);
			return false;
		}

		const DirectX::TexMetadata& meta = image.GetMetadata();
		width = static_cast<uint32_t>(meta.width);
		height = static_cast<uint32_t>(meta.height);

		// 2. GPUへアップロード (ミップマップ対応版)
		if (!UploadTexture(device, commandQueue, image))
		{
			SPAN_ERROR("Failed to upload texture to GPU: %s", filepath.c_str());
			return false;
		}

		// 3. SRV ヒープとビューの作成
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)))) return false;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = meta.format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		// Cubemap判定（DDS読み込み等でCubemapフラグが立っている場合）
		if (meta.miscFlags & DirectX::TEX_MISC_TEXTURECUBE)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = static_cast<UINT>(meta.mipLevels);
			srvDesc.TextureCube.MostDetailedMip = 0;
		}
		else
		{
			srvDesc.Texture2D.MipLevels = static_cast<UINT>(meta.mipLevels);
			srvDesc.Texture2D.MostDetailedMip = 0;
		}

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
		desc.MipLevels = static_cast<UINT16>(mipLevels);
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
		uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
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
		uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
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

	bool Texture::InitializeFromMemory(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const void* data, uint32_t w, uint32_t h, uint32_t bytesPerPixel, DXGI_FORMAT format)
	{
		this->width = w;
		this->height = h;
		this->m_IsHDR = false;
		this->m_FilePath = "MemoryTexture";

		if (!data) return false;

		// 1. GPUへアップロード
		if (!UploadTextureSingle(device, commandQueue, data, width, height, bytesPerPixel, format))
		{
			return false;
		}

		// 2. SRVヒープの作成
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap))))
		{
			SPAN_ERROR("Failed to create SRV Heap for Memory Texture");
			return false;
		}

		// 3. SRVの作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		srvHandleCPU = srvHeap->GetCPUDescriptorHandleForHeapStart();
		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandleCPU);

		return true;
	}

	bool Texture::UploadTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const DirectX::ScratchImage& image)
	{
		const DirectX::TexMetadata& meta = image.GetMetadata();

		// 1. DirectXTexの機能で、最適なテクスチャリソースを生成
		HRESULT hr = DirectX::CreateTexture(device, meta, &resource);
		if (FAILED(hr)) return false;

		// 2. アップロード用バッファの要件を取得
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		hr = DirectX::PrepareUpload(device, image.GetImages(), image.GetImageCount(), meta, subresources);
		if (FAILED(hr)) return false;

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resource.Get(), 0, static_cast<UINT>(subresources.size()));

		// 3. アップロードヒープの作成
		D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
		D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		hr = device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer));
		if (FAILED(hr)) return false;

		// 4. コマンドリストの作成
		ComPtr<ID3D12CommandAllocator> cmdAlloc;
		ComPtr<ID3D12GraphicsCommandList> cmdList;
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));

		// 5. データ転送 (d3dx12.h の UpdateSubresources を活用)
		UpdateSubresources(cmdList.Get(), resource.Get(), uploadBuffer.Get(), 0, 0, static_cast<UINT>(subresources.size()), subresources.data());

		// 6. リソースバリア
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmdList->ResourceBarrier(1, &barrier);

		cmdList->Close();

		// 7. 実行して待機
		ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
		commandQueue->ExecuteCommandLists(1, ppCommandLists);

		ComPtr<ID3D12Fence> fence;
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		commandQueue->Signal(fence.Get(), 1);

		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fenceEvent)
		{
			if (fence->GetCompletedValue() < 1)
			{
				fence->SetEventOnCompletion(1, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
			}
			CloseHandle(fenceEvent);
		}

		return true;
	}

	bool Texture::UploadTextureSingle(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
		const void* initialData, uint64_t w, uint64_t h, uint64_t bytesPerPixel, DXGI_FORMAT format)
	{
		// リソース記述
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = format;
		textureDesc.Width = w;
		textureDesc.Height = static_cast<UINT>(h);
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
		if (fenceEvent)
		{
			if (fence->GetCompletedValue() < 1)
			{
				fence->SetEventOnCompletion(1, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
			}
			CloseHandle(fenceEvent);
		}

		return true;
	}
}
