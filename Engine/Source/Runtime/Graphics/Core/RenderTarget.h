#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	class RenderTarget
	{
	public:
		RenderTarget();
		~RenderTarget();

		// テクスチャリソースとビューを作成
		// width, height: 解像度
		// format: テクスチャフォーマット (通常は R8G8B8A8_UNORM)
		// clearColor: クリア時の色
		bool Initialize(ID3D12Device* device, uint32 width, uint32 height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

		// 解像度変更（ウィンドウリサイズ時に呼ぶ）
		bool Resize(ID3D12Device* device, uint32 width, uint32 height);

		// リソース解放
		void Shutdown();

		// 描画開始前にバリアを設定（SRV -> RTV）
		void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);

		// 描画終了後にバリアを設定（RTV -> SRV: ImGuiで表示するため）
		void TransitionToShaderResource(ID3D12GraphicsCommandList* commandList);

		// 画面クリア
		void Clear(ID3D12GraphicsCommandList* commandList);

		// Getters
		ID3D12Resource* GetResource() const { return resource.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return rtvHandle; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return srvHandle; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return dsvHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRV_GPU() const { return srvHandleGpu; }

		uint32 GetWidth() const { return width; }
		uint32 GetHeight() const { return height; }

	private:
		// ビューの作成
		void CreateViews(ID3D12Device* device);
		void CreateDepthBuffer(ID3D12Device* device);

	private:
		ComPtr<ID3D12Resource> resource;
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		ComPtr<ID3D12DescriptorHeap> srvHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGpu = {};

		// Depth Buffer
		ComPtr<ID3D12Resource> depthResource;
		ComPtr<ID3D12DescriptorHeap> dsvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};

		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
		float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

		D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
		uint32 width = 0;
		uint32 height = 0;
	};
}
