#pragma once
#include "Core/CoreMinimal.h"
#include "Runtime/Platform/Window.h"

namespace Span
{
	class GraphicsContext
	{
	public:
		GraphicsContext();
		~GraphicsContext();

		bool Initialize(Window& window);
		void Shutdown();

		// フレーム開始処理 (コマンドリストのリセット、画面クリア、RTVセット)
		// 戻り値: 描画に使用するコマンドリスト
		ID3D12GraphicsCommandList* BeginFrame();

		// フレーム終了処理 (バリア遷移、コマンド実行、Present、同期)
		void EndFrame();

		// ウィンドウリサイズ時の処理
		void OnResize(uint32 width, uint32 height);

		void SetRenderTargetToBackBuffer(ID3D12GraphicsCommandList* commandList);

		// ゲッター
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
		ID3D12Device* GetDevice() const { return device.Get(); }
		ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
		uint32 GetFrameCount() const { return FrameCount; }
		uint32 GetCurrentFrameIndex() const { return frameIndex; }

		// コマンドリスト実行完了を待機 (リサイズ時や終了時に使用)
		void WaitForGpu();

	private:
		// 内部ヘルパー
		void CreateDevice();
		void CreateCommandQueue();
		void CreateSwapChain(Window& window);
		void CreateRtvHeap();
		void CreateRenderTargets();
		void CreateDepthStencil();
		void CreateCommandResources();
		void CreateSyncObjects();

	private:
		static const uint32 FrameCount = 2;

		// DirectX オブジェクト
		ComPtr<IDXGIFactory4> factory;
		ComPtr<ID3D12Device> device;
		ComPtr<ID3D12CommandQueue> commandQueue;
		ComPtr<IDXGISwapChain3> swapChain;
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		ComPtr<ID3D12DescriptorHeap> dsvHeap;

		// リソース
		ComPtr<ID3D12Resource> renderTargets[FrameCount];
		ComPtr<ID3D12Resource> depthBuffer;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ComPtr<ID3D12GraphicsCommandList> commandList;

		// 同期
		ComPtr<ID3D12Fence> fence;
		uint64 fenceValue = 0;
		HANDLE fenceEvent = nullptr;

		// 状態
		uint32 frameIndex = 0;
		uint32 rtvDescriptorSize = 0;
		D3D12_VIEWPORT viewport = {};
		D3D12_RECT scissorRect = {};

		uint32 width = 0;
		uint32 height = 0;
	};
}
