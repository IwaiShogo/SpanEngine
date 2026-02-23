/*****************************************************************//**
 * @file	SkyboxPass.h
 * @brief	プロシージャルスカイボックス描画パス。
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
#include "Runtime/Scene/EnvironmentSettings.h"
#include "Graphics/Renderer.h"

namespace Span
{
	class Renderer;

	/**
	 * @class	SkyboxPass
	 * @brief	プロシージャルスカイボックス描画パス。
	 */
	class SkyboxPass
	{
	public:
		SkyboxPass() = default;
		~SkyboxPass() { Shutdown(); }

		bool Initialize(ID3D12Device* device);
		void Shutdown();

		/**
		 * @brief	スカイボックスを描画します。RendererのCBVアロケータを使用します。
		 */
		void Render(Renderer* renderer, ID3D12GraphicsCommandList* cmd, const EnvironmentSettings& env, const Matrix4x4& view, const Matrix4x4& proj, const Vector3& camPos, Texture* envCubemap);

	private:
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		Shader* m_shaderVS = nullptr;
		Shader* m_shaderPS = nullptr;
	};
}
