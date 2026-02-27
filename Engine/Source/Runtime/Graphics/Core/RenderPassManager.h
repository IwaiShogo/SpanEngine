/*****************************************************************//**
 * @file	RenderPassManager.h
 * @brief	レンダリングパスのライフサイクルと実行を管理するクラス。
 * 
 * @details	
 * Rendererクラスの肥大化を防ぐため、各種Passの初期化やリサイズを統括します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	class GraphicsContext;
	class GridPass;
	class SkyboxPass;
	class ShadowPass;
	class DepthNormalPass;
	class SSAOPass;
	class SSAOBlurPass;

	/**
	 * @class	RenderPassManager
	 * @brief	🗂 各種レンダリングパスを統括するマネージャー。
	 */
	class RenderPassManager
	{
	public:
		RenderPassManager() = default;
		~RenderPassManager() { Shutdown(); }

		/**
		 * @brief	全てのパスを初期化します。
		 */
		bool Initialize(GraphicsContext* context);

		/**
		 * @brief	全てのパスを破棄します。
		 */
		void Shutdown();

		/**
		 * @brief	画面解像度に依存するパスをリサイズします。
		 */
		void OnResize(ID3D12Device* device, uint32 width, uint32 height);

		// --- Getters ---
		GridPass* GetGridPass() const { return m_gridPass.get(); }
		SkyboxPass* GetSkyboxPass() const { return m_skyboxPass.get(); }
		ShadowPass* GetDirShadowPass() const { return m_dirShadowPass.get(); }
		ShadowPass* GetSpotShadowPass() const { return m_spotShadowPass.get(); }
		ShadowPass* GetPointShadowPass() const { return m_pointShadowPass.get(); }
		DepthNormalPass* GetDepthNormalPass() const { return m_depthNormalPass.get(); }
		SSAOPass* GetSSAOPass() const { return m_ssaoPass.get(); }
		SSAOBlurPass* GetSSAOBlurPass() const { return m_ssaoBlurPass.get(); }

	private:
		std::unique_ptr<GridPass> m_gridPass;
		std::unique_ptr<SkyboxPass> m_skyboxPass;
		std::unique_ptr<ShadowPass> m_dirShadowPass;
		std::unique_ptr<ShadowPass> m_spotShadowPass;
		std::unique_ptr<ShadowPass> m_pointShadowPass;
		std::unique_ptr<DepthNormalPass> m_depthNormalPass;
		std::unique_ptr<SSAOPass> m_ssaoPass;
		std::unique_ptr<SSAOBlurPass> m_ssaoBlurPass;
	};
}
