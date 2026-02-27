/*****************************************************************//**
 * @file	SSAOBlurPass.h
 * @brief	SSAOのノイズを除去するブラーパス
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
#include "Graphics/Core/RenderTarget.h"
#include "Graphics/Renderer.h"

namespace Span
{
	class Renderer;

	/**
	 * @class	SSAOBlurPass
	 * @brief	🌫 SSAOマップのノイズを平滑化するフルスクリーンパス。
	 */
	class SSAOBlurPass
	{
	public:
		SSAOBlurPass() = default;
		~SSAOBlurPass() { Shutdown(); }

		bool Initialize(ID3D12Device* device, uint32 width, uint32 height);
		void Shutdown();
		void Resize(ID3D12Device* device, uint32 width, uint32 height);

		void Execute(Renderer* renderer, ID3D12GraphicsCommandList* cmd, RenderTarget* ssaoMap);

		/// @brief	滑らかになったSSAOマップを取得します。
		RenderTarget* GetBlurredSSAOMap() const { return m_blurredMap; }

	private:
		RenderTarget* m_blurredMap = nullptr;
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		Shader* m_shaderVS = nullptr;
		Shader* m_shaderPS = nullptr;
	};
}
