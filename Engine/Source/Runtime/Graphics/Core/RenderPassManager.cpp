/*****************************************************************//**
 * @file	RenderPassManager.cpp
 * @brief	RenderPassManagerの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "RenderPassManager.h"
#include "Graphics/Core/GraphicsContext.h"
#include "Graphics/Passes/GridPass.h"
#include "Graphics/Passes/SkyboxPass.h"
#include "Graphics/Passes/ShadowPass.h"
#include "Graphics/Passes/DepthNormalPass.h"
#include "Graphics/Passes/SSAOPass.h"
#include "Graphics/Passes/SSAOBlurPass.h"

namespace Span
{
	bool RenderPassManager::Initialize(GraphicsContext* context)
	{
		if (!context) return false;
		auto device = context->GetDevice();
		auto queue = context->GetCommandQueue();
		uint32 w = context->GetViewportWidth();
		uint32 h = context->GetViewportHeight();

		m_gridPass = std::make_unique<GridPass>();
		if (!m_gridPass->Initialize(device)) return false;

		m_skyboxPass = std::make_unique<SkyboxPass>();
		if (!m_skyboxPass->Initialize(device)) return false;

		m_dirShadowPass = std::make_unique<ShadowPass>();
		if (!m_dirShadowPass->Initialize(device, 4096, 4096, 1)) return false;

		m_spotShadowPass = std::make_unique<ShadowPass>();
		if (!m_spotShadowPass->Initialize(device, 1024, 1024, 4)) return false;

		m_pointShadowPass = std::make_unique<ShadowPass>();
		if (!m_pointShadowPass->Initialize(device, 1024, 1024, 6, true)) return false;

		m_depthNormalPass = std::make_unique<DepthNormalPass>();
		if (!m_depthNormalPass->Initialize(device, w, h)) return false;

		m_ssaoPass = std::make_unique<SSAOPass>();
		if (!m_ssaoPass->Initialize(device, queue, w, h)) return false;

		m_ssaoBlurPass = std::make_unique<SSAOBlurPass>();
		if (!m_ssaoBlurPass->Initialize(device, w, h)) return false;
	}

	void RenderPassManager::Shutdown()
	{
		m_gridPass.reset();
		m_skyboxPass.reset();
		m_dirShadowPass.reset();
		m_spotShadowPass.reset();
		m_depthNormalPass.reset();
		m_ssaoPass.reset();
		m_ssaoBlurPass.reset();
	}

	void RenderPassManager::OnResize(ID3D12Device* device, uint32 width, uint32 height)
	{
		if (m_depthNormalPass) m_depthNormalPass->Resize(device, width, height);
		if (m_ssaoPass) m_ssaoPass->Resize(device, width, height);
		if (m_ssaoBlurPass) m_ssaoBlurPass->Resize(device, width, height);
	}
}
