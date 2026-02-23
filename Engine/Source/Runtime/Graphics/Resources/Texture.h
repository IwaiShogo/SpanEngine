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

namespace Span
{
	/**
	 * @class	Texture
	 * @brief	🖼 GPUテクスチャリソース。
	 * 
	 * @details
	 * `stb_image` を使用して画像ファイル(.png, .jpg)を読み込み、DirectX 12テクスチャとしてVRAMに配置します。
	 * 
	 * ### 🔄 Upload Process
	 * 1. **Staging Buffer**: `Upload Heap` (CPUが見えるメモリ) に画像データをコピー。
	 * 2. **Copy Command**: `CopyTextureRegion` で Upload Heap から `Default Heap` (VRAM) へ転送。
	 * 3. **Transition**: リソースの状態を `PIXEL_SHADER_RESOURCE` に変更してシェーダーで使用可能にする。
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
		 * @param	size Cubemapの1面の解像度
		 */
		bool InitializeAsCubemap(ID3D12Device* device, uint32_t size);

		/// @brief	終了処理
		void Shutdown();

		/**
		 * @brief	ImGui表示用のテクスチャIDを取得します。
		 * @return	ImTextureID (GPU Descriptor Handle ptr)
		 */
		void* GetImGuiTextureID() const
		{
			if (srvHeap)
			{
				return (void*)srvHeap->GetGPUDescriptorHandleForHeapStart().ptr;
			}
			return nullptr;
		}

		/// @brief	SRVヒープ上のCPUハンドル (ディスクリプタのコピー元)
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return srvHandleCPU; }

		/// @brief	SRVヒープ自体 (ShaderVisibleではない、保管用)
		ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap.Get(); }

		D3D12_GPU_DESCRIPTOR_HANDLE GetUAV() const { return uavHeap->GetGPUDescriptorHandleForHeapStart(); }

		D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCPU() const { return uavHeap->GetCPUDescriptorHandleForHeapStart(); }

		/// @brief	パスの取得
		const std::string& GetPath() const { return m_FilePath; }
		bool IsHDR() const { return m_IsHDR; }

		AssetHandle Handle = 0;

	private:
		// 画像データをGPUバッファへアップロードするヘルパー関数
		bool UploadTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
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
