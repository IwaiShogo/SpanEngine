/*****************************************************************//**
 * @file	ComputeBuffer.h
 * @brief	Compute Shader 用の Structured Buffer / UAV 管理。
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
	 * @class	ComputeBuffer
	 * @brief	大量の配列データを扱うための Structured Buffer。
	 */
	class ComputeBuffer
	{
	public:
		ComputeBuffer() = default;
		~ComputeBuffer() { Shutdown(); }

		/**
		 * @brief	バッファを初期化します。
		 * @param	isUAV trueならComputeShaderからの書き込みを許可(Default Heap)、falseならCPUからの書き込み専用。
		 */
		bool Initialize(ID3D12Device* device, uint32 elementSize, uint32 elementCount, bool isUAV = false);
		void Shutdown();

		/**
		 * @brief	CPUからデータを書き込みます。
		 */
		void UpdateData(const void* data, size_t dataSize);

		ID3D12Resource* GetResource() const { return m_resource.Get(); }
		uint32 GetElementCount() const { return m_elementCount; }
		uint32 GetElementSize() const { return m_elementSize; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_srvHeap ? m_srvHeap->GetCPUDescriptorHandleForHeapStart() : D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const { return m_uavHeap ? m_uavHeap->GetCPUDescriptorHandleForHeapStart() : D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }; }
		bool IsUAV() const { return m_isUAV; }

	private:
		ComPtr<ID3D12Resource> m_resource;

		// ビューの保存先
		ComPtr<ID3D12DescriptorHeap> m_srvHeap;
		ComPtr<ID3D12DescriptorHeap> m_uavHeap;

		void* m_mappedData = nullptr;
		uint32 m_elementSize = 0;
		uint32 m_elementCount = 0;
		bool m_isUAV = false;
	};
}
