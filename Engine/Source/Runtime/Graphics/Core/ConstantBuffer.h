/*****************************************************************//**
 * @file	ConstantBuffer.h
 * @brief	DirectX 12 定数バッファ (CBV) のラッパークラス
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
	 * @brief	🔢 定数バッファのバイトサイズ計算ヘルパー。
	 * 
	 * @details
	 * DirectX 12の定数バッファサイズは **256バイトの倍数** である必要があります。
	 * この関数は、入力サイズを切り上げます。
	 * 
	 * Example:
	 * - Input: 12 bytes -> Output: 256 bytes
	 * - Input: 256 bytes -> Output: 512 bytes
	 */
	inline uint32 CalcConstantBufferByteSize(uint32 byteSize)
	{
		return (byteSize + 255) & ~255;
	}

	/**
	 * @class	ConstantBuffer
	 * @brief	📦 CPUからGPUへデータを送信するための定数バッファ。
	 * 
	 * @details
	 * シェーダーで使用する `cbuffer` に対応します。
	 * 内部で **Upload Heap** を確保し、CPUからの書き込みを即座にGPUに反映させます。
	 * 
	 * ### ⚠ 注意点
	 * 構造体 `T` のメンバ変数は、HLSLのパッキングルール (16バイト境界) に注意して配置してください。
	 * 
	 * ### 📝 Usage
	 * ```cpp
	 * struct SceneData { Matrix4x4 ViewProj; };
	 * ConstantBuffer<SceneData> cb;
	 * cb.Initialize(device);
	 * cb.Data.ViewProj = camera.GetViewProj();	// データを書き込む
	 * // 自動的にGPUメモリへマップされているため、明示的な転送関数は不要
	 * ```
	 * @tparam	T バッファに格納する構造体の型
	 */ 
	template <typename T>
	class ConstantBuffer
	{
	public:
		/// @brief	GPUに転送される実データへの参照。これに書き込むだけで反映されます。
		T Data;

		ConstantBuffer() = default;

		~ConstantBuffer()
		{
			Shutdown();
		}

		/**
		 * @brief	定数バッファリソースを作成します。
		 * @param	device D3D12デバイス
		 * @return	成功なら true
		 */
		bool Initialize(ID3D12Device* device)
		{
			// 256バイトアライメント計算
			uint32 sizeInBytes = CalcConstantBufferByteSize(sizeof(T));

			// 1. アップロードヒープを作成 (CPUから書き込む用)
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			// 2. リソース記述子の設定
			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Alignment = 0;
			resourceDesc.Width = sizeInBytes;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			// 3. リソース生成
			if (FAILED(device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&resource))))
			{
				SPAN_ERROR("Failed to create Constant Buffer!");
				return false;
			}

			// 4. メモリマッピング (永続マップ)
			D3D12_RANGE readRange = { 0, 0 };
			if (FAILED(resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData))))
			{
				SPAN_ERROR("Failed to map Constant Buffer!");
				return false;
			}

			// 初期化
			// ZeroMemory(mappedData, sizeInBytes);

			return true;
		}

		/// @brief	リソースを解放します。
		void Shutdown()
		{
			if (resource)
			{
				resource->Unmap(0, nullptr);
				mappedData = nullptr;
				resource.Reset();
			}
		}

		/**
		 * @brief	現在の `Data` の内容をGPUバッファへコピーします。
		 * @note	毎フレーム描画前に呼び出してください。
		 */
		void Update(const T& data)
		{
			if (mappedData)
			{
				memcpy(mappedData, &data, sizeof(T));
			}
		}

		/// @brief	GPU仮想アドレスを取得 (SetGraphicsRootConstantBufferView用)
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
		{
			if (resource)
			{
				return resource->GetGPUVirtualAddress();
			}
			return 0;
		}

	private:
		ComPtr<ID3D12Resource> resource;	///< GPUリソース
		T* mappedData = nullptr;			///< CPU側から見れるメモリアドレス
	};
}

