#pragma once
#include "Core/CoreMinimal.h"
#include "Platform/Window.h"
#include "Core/Shader.h"
#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Core/ConstantBuffer.h"
#include "Core/Math/SpanMath.h"

namespace Span
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		// 初期化
		bool Initialize(Window& window);

		// 終了処理
		void Shutdown();

		// 描画開始
		void BeginFrame();

		// 描画終了
		void EndFrame();

		// 外部から呼ばれる描画関数
		void DrawMesh(Mesh* mesh, Material* material, const Matrix4x4& worldMatrix);

		// カメラ行列の設定
		void SetCamera(const Matrix4x4& view, const Matrix4x4 projection);

		// リサイズ
		void OnResize(uint32 width, uint32 height);

		// デバイスへのアクセス
		ID3D12Device* GetDevice() const { return device.Get(); }

		// コマンドキュー
		ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }

		// フレームバッファの数を返す
		int GetFrameCount() const { return FrameCount; }

		// コマンドリスト
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }

	private:
		// --- DX12 Core Objects ---
		ComPtr<ID3D12Device> device;				// GPUデバイス
		ComPtr<IDXGIFactory4> factory;				// DXGIファクトリー
		ComPtr<ID3D12CommandQueue> commandQueue;	// コマンドキュー
		ComPtr<IDXGISwapChain3> swapChain;			// スワップチェーン (画面バッファ)

		// --- Descriptor Heap (RTV: Render Target View) ---
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		uint32 rtvDescriptorSize = 0;

		// --- Back Buffers ---
		static const uint32 FrameCount = 2;	// ダブルバッファリング
		ComPtr<ID3D12Resource> renderTargets[FrameCount];
		uint32 frameIndex = 0;

		// --- Command Objects ---
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ComPtr<ID3D12GraphicsCommandList> commandList;

		// --- Synchronization (Fence) ---
		ComPtr<ID3D12Fence> fence;
		uint64 fenceValue = 0;
		HANDLE fenceEvent = nullptr;

		// --- Viewport & Scissor ---
		D3D12_VIEWPORT viewport;
		D3D12_RECT scissorRect;

		// --- パイプライン関連 ---
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipelineState;				// 不透明用 (Opaque)
		ComPtr<ID3D12PipelineState> pipelineStateTransparent;	// 透明用 (Transparent)

		// --- シェーダー ---
		Shader* vs = nullptr;
		Shader* ps = nullptr;

		// --- カメラ行列 ---
		Matrix4x4 viewMatrix;
		Matrix4x4 projectionMatrix;

		// 内部ヘルパー関数
		void LoadPipeline(Window& window);
		bool LoadAssets();
		void WaitForPreviousFrame();

		bool CreateRootSignature();
		bool CreatePipelineState();

		void CreateDepthBuffer(uint32 width, uint32 height);

		struct TransformData
		{
			Matrix4x4 MVP;
			Matrix4x4 World;
		};
		ComPtr<ID3D12Resource> constantBuffer;
		UINT8* mappedConstantBuffer = nullptr;
		uint32 constantBufferIndex = 0;	// フレームで何個目か

		// 最大描画数
		static const uint32 MAX_OBJECTS = 2000;
		// 1つあたりのサイズ
		static const uint32 CB_OBJ_SIZE = (sizeof(TransformData) + 255) & ~255;

		// --- Depth Stencil Buffer (Z-Buffer) ---
		ComPtr<ID3D12DescriptorHeap> dsvHeap;	// Depth Stencil View Heap
		ComPtr<ID3D12Resource> depthBuffer;		// 深度情報のメモリ
		uint32 dsvDescriptorSize = 0;
	};
}
