#include "RenderTarget.h"

namespace Span
{
	RenderTarget::RenderTarget()
	{
	}

	RenderTarget::~RenderTarget()
	{
		Shutdown();
	}

	bool RenderTarget::Initialize(ID3D12Device* device, uint32 width, uint32 height, DXGI_FORMAT format)
	{
		if (!device)
		{
			SPAN_ERROR("[RenderTarget] Initialize failed: Device is NULL!");
			return false;
		}

		this->width = width;
		this->height = height;
		this->format = format;

		HRESULT hr = S_OK;

		// 1. RTV Heap
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
		if (FAILED(hr))
		{
			SPAN_ERROR("[RenderTarget] Failed to create RTV Heap! HRESULT: 0x%08X", hr);
			return false;
		}
		rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

		// 2. SRV Heap
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
		if (FAILED(hr))
		{
			SPAN_ERROR("[RenderTarget] Failed to create SRV Heap! HRESULT: 0x%08X", hr);
			return false;
		}
		srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
		srvHandleGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();

		// 3. リソースの作成
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Width = width;
		resDesc.Height = height;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = format;
		resDesc.SampleDesc.Count = 1;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		// クリアカラー設定
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = format;
		clearValue.Color[0] = 0.1f;
		clearValue.Color[1] = 0.1f;
		clearValue.Color[2] = 0.1f;
		clearValue.Color[3] = 1.0f;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		// リソース生成
		if (FAILED(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 初期状態
			&clearValue,
			IID_PPV_ARGS(&resource))))
		{
			SPAN_ERROR("Failed to create RenderTarget Resource");
			return false;
		}

		currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		// 4. ビュー(RTV/SRV)の作成

		// RTV作成
		device->CreateRenderTargetView(resource.Get(), nullptr, rtvHandle);

		// SRV作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);


		// 5. 深度バッファの作成
		CreateDepthBuffer(device);

		return true;
	}

	void RenderTarget::Shutdown()
	{
		resource.Reset();
		depthResource.Reset();
		rtvHeap.Reset();
		srvHeap.Reset();
		dsvHeap.Reset();
	}

	void RenderTarget::Resize(ID3D12Device* device, uint32 newWidth, uint32 newHeight)
	{
		//if (newWidth == 0 || newHeight == 0) return false;

		//// 最小サイズ保証
		//newWidth = std::max(newWidth, 1u);
		//newHeight = std::max(newHeight, 1u);

		//if (width == newWidth && height == newHeight && resource) return true;

		//width = newWidth;
		//height = newHeight;

		//// 既存のリソースがあれば解放
		//resource.Reset();
		//depthResource.Reset();

		//// リソース記述
		//D3D12_RESOURCE_DESC resDesc = {};
		//resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		//resDesc.Alignment = 0;
		//resDesc.Width = width;
		//resDesc.Height = height;
		//resDesc.DepthOrArraySize = 1;
		//resDesc.MipLevels = 1;
		//resDesc.Format = format;
		//resDesc.SampleDesc.Count = 1;
		//resDesc.SampleDesc.Quality = 0;
		//resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		//resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		//// クリア値の設定
		//D3D12_CLEAR_VALUE optClear;
		//optClear.Format = format;
		//memcpy(optClear.Color, clearColor, sizeof(float) * 4);

		//// デフォルトヒープ（VRAM）に作成
		//D3D12_HEAP_PROPERTIES heapProps = {};
		//heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		//heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		//heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//heapProps.CreationNodeMask = 1;
		//heapProps.VisibleNodeMask = 1;

		//// 初期状態は PIXEL_SHADER_RESOURCE にしておく（ImGuiが表示するため）
		//currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		//HRESULT hr = device->CreateCommittedResource(
		//	&heapProps,
		//	D3D12_HEAP_FLAG_NONE,
		//	&resDesc,
		//	currentState,
		//	&optClear,
		//	IID_PPV_ARGS(&resource)
		//);

		//if (FAILED(hr))
		//{
		//	SPAN_ERROR("RenderTarget: Failed to create texture resource!");
		//	return false;
		//}

		//CreateViews(device);
		//CreateDepthBuffer(device);

		//return true;

		// サイズが変わっていない、または無効なサイズなら何もしない
		if ((width == newWidth && height == newHeight) || newWidth == 0 || newHeight == 0)
		{
			return;
		}

		// 既存のリソースを解放
		Shutdown();

		// 新しいサイズで初期化
		Initialize(device, newWidth, newHeight);
	}

	void RenderTarget::CreateViews(ID3D12Device* device)
	{
		if (!resource) return;

		// RTV 作成
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		device->CreateRenderTargetView(resource.Get(), &rtvDesc, rtvHandle);

		// SRV 作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
	}

	void RenderTarget::CreateDepthBuffer(ID3D12Device* device)
	{
		// 1. DSV Heap
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)))) return;

		dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		// 2. Depth Resource
		D3D12_RESOURCE_DESC depthDesc = {};
		depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Width = width;
		depthDesc.Height = height;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Format = DXGI_FORMAT_D32_FLOAT; // 通常はD32_FLOAT
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_CLEAR_VALUE depthClear = {};
		depthClear.Format = DXGI_FORMAT_D32_FLOAT;
		depthClear.DepthStencil.Depth = 1.0f;
		depthClear.DepthStencil.Stencil = 0;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		if (FAILED(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClear,
			IID_PPV_ARGS(&depthStencil))))
		{
			SPAN_ERROR("Failed to create Depth Stencil Resource");
			return;
		}

		// --- 【重要】3. DSVの作成 ---
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(depthStencil.Get(), &dsvDesc, dsvHandle);
	}

	void RenderTarget::TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList)
	{
		if (!resource) return;

		if (currentState == D3D12_RESOURCE_STATE_RENDER_TARGET) return;

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource.Get();
		barrier.Transition.StateBefore = currentState;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		commandList->ResourceBarrier(1, &barrier);
		currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	void RenderTarget::TransitionToShaderResource(ID3D12GraphicsCommandList* commandList)
	{
		if (!resource) return;

		if (currentState == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) return;

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource.Get();
		barrier.Transition.StateBefore = currentState;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		commandList->ResourceBarrier(1, &barrier);
		currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}

	void RenderTarget::Clear(ID3D12GraphicsCommandList* commandList)
	{
		if (rtvHandle.ptr == 0) return;

		// レンダーターゲット状態であることを前提とする（事前にTransitionを呼ぶこと）
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}
