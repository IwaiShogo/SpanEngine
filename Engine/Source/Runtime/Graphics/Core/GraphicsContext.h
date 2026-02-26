/*****************************************************************//**
 * @file	GraphicsContext.h
 * @brief	DirectX 12 デヴァイスとスワップチェーンの管理クラス。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Runtime/Platform/Window.h"

namespace Span
{
	/**
	 * @class	GraphicsContext
	 * @brief	🎮 GPUコンテキストのラッパー。描画パイプラインの基盤。
	 * 
	 * @details
	 * Device, CommandQueue, SwapChain など主要なDX12オブジェクトを管理します。
	 * また、フレームの開始(Begin)・終了(End)処理や、GPUとの同期(Fence)も担当します。
	 * 
	 * ### 🖼 Swap Chain (Double Buffering)
	 * | Buffer 0 (Font) | Buffer 1 (Back) |
	 * | :---:           | :---: |
	 * | Displaying 📺  | Rendering 🖌 |
	 * 
	 * VSyncに合わせてフリップすることでティアリングを防ぎます。
	 */
	class GraphicsContext
	{
	public:
		GraphicsContext();
		~GraphicsContext();

		/**
		 * @brief	DX12デバイスとスワップチェーンを初期化します。
		 * @param	window 描画対象のウィンドウ
		 * @return	成功なら true
		 */
		bool Initialize(Window& window);

		/// @brief	全リソースを解放し、GPU処理の完了を待機して終了します。
		void Shutdown();

		/**
		 * @brief	フレーム描画の開始処理。
		 * コマンドアロケータのリセット、バックバッファのクリア、RTVの設定を行います。
		 * @return	描画コマンドを積むための CommandList
		 */
		ID3D12GraphicsCommandList* BeginFrame();

		/**
		 * @brief	フレーム描画の終了処理。
		 * リソースバリアの遷移、コマンドリストの実行、Present(画面更新)、次フレームへの待機を行います。
		 */
		void EndFrame();

		/**
		 * @brief	ウィンドウサイズ時のバッファ再生成処理
		 */
		void OnResize(uint32 width, uint32 height);

		/**
		 * @brief	現在のレンダーターゲットをバックバッファ (スワップチェーン) に戻します。
		 * オフスクリーン描画からメイン描画に戻る際に使用します。
		 */
		void SetRenderTargetToBackBuffer(ID3D12GraphicsCommandList* commandList);

		// 📊 Getters
		// ============================================================
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;
		ID3D12Device* GetDevice() const { return device.Get(); }
		ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
		uint32 GetFrameCount() const { return FrameCount; }
		uint32 GetCurrentFrameIndex() const { return frameIndex; }

		uint32 GetViewportWidth() const { return width; }
		uint32 GetViewportHeight() const { return height; }

		/**
		 * @brief	GPUの処理完了をCPU側で待機します。
		 * @warning	パフォーマンス定価の原因になるため、初期化・終了・リサイズ時のみ使用してください。
		 */
		void WaitForGpu();

	private:
		// --- Initialization Helpers ---
		void CreateDevice();
		void CreateCommandQueue();
		void CreateSwapChain(Window& window);
		void CreateRtvHeap();
		void CreateRenderTargets();
		void CreateDepthStencil();
		void CreateCommandResources();
		void CreateSyncObjects();

	private:
		/// @brief	ダブルバッファリング用のフレーム数
		static const uint32 FrameCount = 2;

		// --- DirectX Objects ---
		ComPtr<IDXGIFactory4> factory;
		ComPtr<ID3D12Device> device;
		ComPtr<ID3D12CommandQueue> commandQueue;
		ComPtr<IDXGISwapChain3> swapChain;
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		ComPtr<ID3D12DescriptorHeap> dsvHeap;

		// --- Resources ---
		ComPtr<ID3D12Resource> renderTargets[FrameCount];
		ComPtr<ID3D12Resource> depthBuffer;

		// --- Commands ---
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ComPtr<ID3D12GraphicsCommandList> commandList;

		// --- Synchronization (Fence) ---
		ComPtr<ID3D12Fence> fence;
		uint64 fenceValue = 0;
		HANDLE fenceEvent = nullptr;

		// --- State ---
		uint32 frameIndex = 0;
		uint32 rtvDescriptorSize = 0;

		uint32 width = 0;
		uint32 height = 0;
		D3D12_VIEWPORT viewport = {};
		D3D12_RECT scissorRect = {};
	};
}

