#include "Texture.h"

// 画像読み込みライブラリの実装を定義
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Span
{
	Texture::Texture() {}
	Texture::~Texture() { Shutdown(); }

	void Texture::Shutdown()
	{
		resource.Reset();
		uploadBuffer.Reset(); // アップロードが終われば解放して良いが、今回は保持
		srvHeap.Reset();
	}

	bool Texture::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const std::string& filepath)
	{
		SPAN_LOG("Loading Texture: %s", filepath.c_str());

		m_FilePath = filepath;

		// 1. stb_image で画像読み込み
		int w, h, channels;
		// RGBA (4チャンネル) で強制的に読み込む
		unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &channels, 4);

		if (!data)
		{
			SPAN_ERROR("Failed to load image: %s", filepath.c_str());
			return false;
		}

		width = static_cast<uint32_t>(w);
		height = static_cast<uint32_t>(h);

		// 2. GPUへアップロード
		if (!UploadTexture(device, commandQueue, data, width, height, 4))
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
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		srvHandleCPU = srvHeap->GetCPUDescriptorHandleForHeapStart();
		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandleCPU);

		return true;
	}

	bool Texture::UploadTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
		const void* initialData, uint64_t w, uint64_t h, uint64_t bytesPerPixel)
	{
		// リソース記述
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = w;
		textureDesc.Height = h;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		// 1. デフォルトヒープ（VRAM）にテクスチャリソースを作成
		// 初期状態は COPY_DEST (コピー先) にする
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
		// (本来はRendererのコマンドリストを使うべきだが、初期化用として一時的に作成)
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

		// 実行して待機 (簡易同期)
		ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
		commandQueue->ExecuteCommandLists(1, ppCommandLists);

		// フェンス同期 (待機)
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
