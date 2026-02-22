/*****************************************************************//**
 * @file	GridPass.h
 * @brief	エディタ用グリッドの描画パス。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Core/Shader.h"
#include "Graphics/Resources/Mesh.h"

namespace Span
{
	/**
	 * @class	GridPass
	 * @brief	📐 エディタ空間の無限グリッドとXYZ軸を描画するパス。
	 */
	class GridPass
	{
	public:
		GridPass() = default;
		~GridPass() { Shutdown(); }

		bool Initialize(ID3D12Device* device);
		void Shutdown();

		/**
		 * @brief	グリッドを描画します。
		 * @param	cmd コマンドリスト
		 * @param	sceneCbAddress カメラ情報が含まれるCBVのGPUアドレス (b0)
		 */
		void Render(ID3D12GraphicsCommandList* cmd, D3D12_GPU_VIRTUAL_ADDRESS sceneCbAddress);

	private:
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		Shader* m_shaderVS = nullptr;
		Shader* m_shaderPS = nullptr;
		Mesh* m_planeMesh = nullptr;
	};
}
