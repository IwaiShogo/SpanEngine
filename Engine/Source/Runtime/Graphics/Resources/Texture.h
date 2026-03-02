/*****************************************************************//**
 * @file	Texture.h
 * @brief	画像読み込みとテクスチャリソース管理。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Runtime/Resource/AssetMetadata.h"
#include <DirectXTex.h>

namespace Span
{
	/**
	 * @class	Texture
	 * @brief	🖼 GPUテクスチャリソース。
	 * 
	 * @details
	 * `DirectXTex` を使用して画像ファイル(.dds, .png, .jpg, .hdr)を読み込み、DirectX 12テクスチャとしてVRAMに配置します。
	 * DDSフォーマットによる高速読み込みとミップマップに完全対応しています。
	 */
	class Texture
	{
	public:
		Texture() = default;
		~Texture() { Shutdown(); }

		/**
		 * @brief	画像ファイルからテクスチャを作成します。
		 * @param	device D3D12デバイス
		 * @param	commandQueue 転送コマンドを実行するキュー
		 * @param	filepath ファイルパス (Assets/...)
		 * @return	成功ならtrue
		 */
		bool Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const std::string& filepath);

		/**
		 * @brief	Compute Shaderで書き込むための「空のCubemap」を生成します。
		 */
		bool InitializeAsCubemap(ID3D12Device* device, uint32_t size, uint32_t mipLevels = 1);

		/**
		 * @brief	Compute Shaderで書き込むための「空の2Dテクスチャ」を生成します。
		 */
		bool InitializeAsTexture2D(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format);

		/**
		 * @brief	メモリ上のピクセルデータからテクスチャを作成します。
		 */
		bool InitializeFromMemory(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
			const void* data, uint32_t width, uint32_t height, uint32_t bytesPerPixel,
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

		/// @brief	終了処理
		void Shutdown();

		/**
		 * @brief	ImGui表示用のテクスチャIDを取得します。
		 * @return	ImTextureID (GPU Descriptor Handle ptr)
		 */
		void* GetImGuiTextureID() const
		{
			/*if (srvHeap)
			{
				return (void*)srvHeap->GetGPUDescriptorHandleForHeapStart().ptr;
			}*/
			return nullptr;
		}

		/// @brief	SRVヒープ上のCPUハンドル (ディスクリプタのコピー元)
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return srvHandleCPU; }

		/// @brief	SRVヒープ自体 (ShaderVisibleではない、保管用)
		ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap.Get(); }

		D3D12_GPU_DESCRIPTOR_HANDLE GetUAV() const { return uavHeap->GetGPUDescriptorHandleForHeapStart(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCPU() const { return uavHeap->GetCPUDescriptorHandleForHeapStart(); }

		/// @brief	DirectXリソースの取得
		ID3D12Resource* GetResource() const { return resource.Get(); }

		/// @brief	パスの取得
		const std::string& GetPath() const { return m_FilePath; }
		bool IsHDR() const { return m_IsHDR; }

		AssetHandle Handle = 0;

	private:
		// ミップマップ対応の高度なアップロード関数 (DirectXTex対応版)
		bool UploadTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const DirectX::ScratchImage& image);

		// 1枚絵用のレガシーアップロード関数 (MemoryTexture用)
		bool UploadTextureSingle(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
			const void* initialData, uint64_t width, uint64_t height, uint64_t bytesPerPixel, DXGI_FORMAT format);

	private:
		ComPtr<ID3D12Resource> resource;		///< テクスチャ本体 (VRAM)
		ComPtr<ID3D12Resource> uploadBuffer;	///< アップロード用の一時バッファ (Upload Heap)
		ComPtr<ID3D12DescriptorHeap> srvHeap;	///< SRV用デスクリプタヒープ (このテクスチャ専用)
		ComPtr<ID3D12DescriptorHeap> uavHeap;	///< UAV用ヒープを追加

		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = {};

		uint32_t width = 0;
		uint32_t height = 0;
		bool m_IsHDR = false;
		std::string m_FilePath;
	};
}
