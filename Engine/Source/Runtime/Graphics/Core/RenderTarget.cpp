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
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
		if (FAILED(hr))
		{
			SPAN_ERROR("[RenderTarget] Failed to create SRV Heap! HRESULT: 0x%08X", hr);
			return false;
		}
		srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

		// 3. リソースの作成
		CreateViews(device);

		// 4. 深度バッファの作成
		CreateDepthBuffer(device);

		// 初期状態
		currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		return true;
	}

	void RenderTarget::Shutdown()
	{
		resource.Reset();
		rtvHeap.Reset();
		srvHeap.Reset();
		dsvHeap.Reset();
		depthBuffer.Reset();
	}

	void RenderTarget::Resize(ID3D12Device* device, uint32 width, uint32 height)
	{
		if (this->width == width && this->height == height) return;

		this->width = width;
		this->height = height;

		// リソースを解放して再作成
		resource.Reset();
		depthBuffer.Reset();
		dsvHeap.Reset();

		CreateViews(device);
		CreateDepthBuffer(device);
	}

	void RenderTarget::CreateViews(ID3D12Device* device)
	{
		// リソース記述
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Width = width;
		resDesc.Height = height;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = format;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		// クリア値
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = format;
		clearValue.Color[0] = 0.1f;
		clearValue.Color[1] = 0.1f;
		clearValue.Color[2] = 0.1f;
		clearValue.Color[3] = 1.0f;

		D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };

		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 初期状態
			&clearValue,
			IID_PPV_ARGS(&resource)
		);

		if (FAILED(hr))
		{
			SPAN_ERROR("[RenderTarget] Failed to create Committed Resource!");
			return;
		}

		// RTV 作成
		device->CreateRenderTargetView(resource.Get(), nullptr, rtvHandle);

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
		// DSV Heap
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
		dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		// Depth Resource
		D3D12_RESOURCE_DESC depthDesc = {};
		depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Width = width;
		depthDesc.Height = height;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthClear = {};
		depthClear.Format = DXGI_FORMAT_D32_FLOAT;
		depthClear.DepthStencil.Depth = 1.0f;
		depthClear.DepthStencil.Stencil = 0;

		D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };

		device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClear,
			IID_PPV_ARGS(&depthBuffer)
		);

		// DSV 作成
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHandle);
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
		const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}
