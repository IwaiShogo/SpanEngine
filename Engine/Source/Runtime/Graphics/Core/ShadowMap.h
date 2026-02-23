/*****************************************************************//**
 * @file	ShadowMap.h
 * @brief	シャドウマッピング用の深度テクスチャ管理。
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
	 * @class	ShadowMap
	 * @brief	🌑 影の判定に使用する深度(Depth)専用のテクスチャ
	 */
	class ShadowMap
	{
	public:
		ShadowMap() = default;
		~ShadowMap() { Shutdown(); }

		/**
		 * @brief	シャドウマップを初期化します。
		 * @param	device DX12デバイス
		 * @param	width テクスチャの幅
		 * @param	height テクスチャの高さ
		 */
		bool Initialize(ID3D12Device* device, uint32 width, uint32 height, uint32 arraySize = 1, bool isCube = false);
		void Shutdown();

		// --- Getters ---
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(uint32 sliceIndex = 0) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const;
		ID3D12DescriptorHeap* GetSRVHeap() const { return m_srvHeap.Get(); }
		ID3D12Resource* GetResource() const { return m_resource.Get(); }

		uint32 GetWidth() const { return m_width; }
		uint32 GetHeight() const { return m_height; }

	private:
		uint32 m_width = 0;
		uint32 m_height = 0;
		uint32 m_arraySize = 1;
		uint32 m_dsvDescriptorSize = 0;

		ComPtr<ID3D12Resource> m_resource;
		ComPtr<ID3D12DescriptorHeap> m_srvHeap;
		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	};
}
