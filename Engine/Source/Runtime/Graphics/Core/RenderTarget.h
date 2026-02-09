/*****************************************************************//**
 * @file	RenderTarget.h
 * @brief	レンダリングターゲット (RTV) とシェーダーリソース (SRV) の管理。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	/**
	 * @class	RenderTarget
	 * @brief	🎯 描画対象となるテクスチャリソース。
	 *
	 * @details
	 * 「描画先 (RTV)」としても、「テクスチャ (SRV)」としても使用できるリソースを管理します。
	 * ImGui経由でテクスチャとして表示することで実現されています。
	 *
	 * ### 🔄 Resource Barrier State Flow
	 * 1. **RT State**: 描画中 (`D3D12_RESOURCE_STATE_RENDER_TARGET`)
	 * 2. **Barrier**: `TransitionToShaderResource()`
	 * 3. **SRV State**: ImGui表示中 (`D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE`)
	 * 4. **Barrier**: `TransitionToRenderTarget()`
	 * 5. (Loop)
	 */
	class RenderTarget
	{
	public:
		RenderTarget();
		~RenderTarget();

		/**
		 * @brief	レンダーターゲットリソースを作成します。
		 * @param	device D3D12デバイス
		 * @param	width 幅
		 * @param	height 高さ
		 * @param	format ピクセルフォーマット (Default: R8G8B8A8_UNORM)
		 */
		bool Initialize(ID3D12Device* device, uint32 width, uint32 height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

		/// @brief	リソースを解放
		void Shutdown();

		/// @brief	解像度を変更し、リソースを作り直します。
		void Resize(ID3D12Device* device, uint32 width, uint32 height);

		// 🔄 State Transition Barriers
		// ============================================================

		/**
		 * @brief	リソースの状態を「描画先 (Render Target)」に変更します。
		 * 描画コマンドを発行する前に必ず呼んでください。
		 */
		void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);

		/**
		 * @brief	リソースの状態を「読み取り専用テクスチャ (Shader Resource)」に変更します。
		 * ImGuiなどで表示する前に読んでください。
		 */
		void TransitionToShaderResource(ID3D12GraphicsCommandList* commandList);

		// 🖌 Operations
		// ============================================================

		/// @brief	画面を指定色でクリアします。
		void Clear(ID3D12GraphicsCommandList* commandList);

		// 📊 Getters
		// ============================================================

		ID3D12Resource* GetResource() const { return resource.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return rtvHandle; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return srvHandle; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return dsvHandle; }

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

		// Depth Stencil
		ComPtr<ID3D12DescriptorHeap> dsvHeap;
		ComPtr<ID3D12Resource> depthBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};

		uint32 width = 0;
		uint32 height = 0;
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

		D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
	};
}

