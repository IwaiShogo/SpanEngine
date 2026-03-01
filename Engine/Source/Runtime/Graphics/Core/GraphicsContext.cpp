#include "GraphicsContext.h"

namespace Span
{
	GraphicsContext::GraphicsContext()
	{
	}

	GraphicsContext::~GraphicsContext()
	{
		Shutdown();
	}

	bool GraphicsContext::Initialize(Window& window)
	{
		width = window.GetWidth();
		height = window.GetHeight();

		// ビューポート設定 & シザー矩形の設定
		viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
		scissorRect = { 0, 0, static_cast<long>(width), static_cast<long>(height) };

		// DX12 パイプライン構築
		try
		{
			CreateDevice();				// 1. デバイス作成 (論理GPU)
			CreateCommandQueue();		// 2. コマンドキュー作成
			CreateSwapChain(window);	// 3. スワップチェーン作成 (ウィンドウとの紐づけ)
			CreateRtvHeap();			// 4. RTVヒープ (ディスクリプタ置き場)
			CreateRenderTargets();		// 5. バックバッファの取得
			CreateDepthStencil();		// 6. 深度バッファ
			CreateCommandResources();	// 7. コマンドリスト作成
			CreateSyncObjects();		// 8. フェンス作成
		}
		catch (...)
		{
			SPAN_ERROR("DirectX 12 Initialization Failed.");
			return false;
		}

		SPAN_LOG("GraphicsContext Initialized Successfully (DirectX 12)");
		return true;
	}

	void GraphicsContext::Shutdown()
	{
		// GPUがまだ処理中かもしれないので、完全に終わるまで待つ
		WaitForGpu();

		if (fenceEvent == nullptr) return;

		CloseHandle(fenceEvent);
		fenceEvent = nullptr;
	}

	void GraphicsContext::CreateDevice()
	{
		uint32 dxgiFactoryFlags = 0;
#if defined(_DEBUG)
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif
		CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

#if defined(_DEBUG)
		// 警告の抑制
		ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(device.As(&infoQueue)))
		{
			D3D12_MESSAGE_ID hide[] =
			{
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
			};
			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			infoQueue->AddStorageFilterEntries(&filter);
		}
#endif
	}

	void GraphicsContext::CreateCommandQueue()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
	}

	void GraphicsContext::CreateSwapChain(Window& window)
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		ComPtr<IDXGISwapChain1> swapChain1;
		factory->CreateSwapChainForHwnd(
			commandQueue.Get(),
			window.GetHandle(),
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1
		);
		swapChain1.As(&swapChain);
		frameIndex = swapChain->GetCurrentBackBufferIndex();
	}

	void GraphicsContext::CreateRtvHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	void GraphicsContext::CreateRenderTargets()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (uint32 i = 0; i < FrameCount; i++)
		{
			swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
			device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}
	}

	void GraphicsContext::CreateDepthStencil()
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

		D3D12_RESOURCE_DESC depthDesc = {};
		depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Width = width;
		depthDesc.Height = height;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear = {};
		optClear.Format = DXGI_FORMAT_D32_FLOAT;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_DEFAULT };

		device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &optClear,
			IID_PPV_ARGS(&depthBuffer)
		);

		device->CreateDepthStencilView(depthBuffer.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void GraphicsContext::CreateCommandResources()
	{
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
		commandList->Close();
	}

	void GraphicsContext::CreateSyncObjects()
	{
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		fenceValue = 1;
		fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		WaitForGpu();
	}

	ID3D12GraphicsCommandList* GraphicsContext::BeginFrame()
	{
		commandAllocator->Reset();
		commandList->Reset(commandAllocator.Get(), nullptr);

		// バックバッファを書き込み可能に遷移
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = renderTargets[frameIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);

		// ビューポート設定
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		// ターゲットセット & クリア
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += frameIndex * rtvDescriptorSize;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		// 背景クリア色 (Cornflower Blue)
		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		return commandList.Get();
	}

	void GraphicsContext::EndFrame()
	{
		// 表示用に遷移
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = renderTargets[frameIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList->ResourceBarrier(1, &barrier);

		commandList->Close();

		// 実行
		ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(1, ppCommandLists);

		// 表示
		swapChain->Present(1, 0);

		// 同期
		const uint64 fenceVal = fenceValue;
		commandQueue->Signal(fence.Get(), fenceVal);
		fenceValue++;

		if (fence->GetCompletedValue() < fenceVal)
		{
			fence->SetEventOnCompletion(fenceVal, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		frameIndex = swapChain->GetCurrentBackBufferIndex();
	}

	void GraphicsContext::WaitForGpu()
	{
		const uint64 fenceVal = fenceValue;
		commandQueue->Signal(fence.Get(), fenceVal);
		fenceValue++;

		if (fence->GetCompletedValue() < fenceVal)
		{
			fence->SetEventOnCompletion(fenceVal, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
	}

	void GraphicsContext::OnResize(uint32 w, uint32 h)
	{
		if (w == 0 || h == 0) return;

		WaitForGpu();

		width = w;
		height = h;

		for (int i = 0; i < FrameCount; ++i) renderTargets[i].Reset();
		depthBuffer.Reset();

		swapChain->ResizeBuffers(FrameCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		frameIndex = swapChain->GetCurrentBackBufferIndex();

		CreateRenderTargets();
		CreateDepthStencil();

		viewport.Width = static_cast<float>(width);
		viewport.Height = static_cast<float>(height);
		scissorRect.right = static_cast<long>(width);
		scissorRect.bottom = static_cast<long>(height);
	}

	void GraphicsContext::SetRenderTargetToBackBuffer(ID3D12GraphicsCommandList* commandList)
	{
		// ターゲットをバックバッファに戻す
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += frameIndex * rtvDescriptorSize;

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// ビューポートも画面全体に戻す
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GraphicsContext::GetCurrentBackBufferRTV() const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += frameIndex * rtvDescriptorSize;
		return handle;
	}
}
