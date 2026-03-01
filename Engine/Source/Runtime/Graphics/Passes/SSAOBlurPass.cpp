/*****************************************************************//**
 * @file	SSAOBlurPass.cpp
 * @brief	SSAOBlurPassの実装。
 *
 * @details
 * ------------------------------------------------------------
 * @author	SpanEngine
 * ------------------------------------------------------------
 *********************************************************************/

#include "SSAOBlurPass.h"

namespace Span
{
	bool SSAOBlurPass::Initialize(ID3D12Device* device, uint32 width, uint32 height)
	{
		m_blurredMap = new RenderTarget();
		const float whiteClear[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		if (!m_blurredMap->Initialize(device, width, height, DXGI_FORMAT_R8_UNORM, whiteClear)) return false;

		m_shaderVS = new Shader();
		if (!m_shaderVS->Load(L"SSAOBlur.hlsl", ShaderType::Vertex, "VSMain")) return false;

		m_shaderPS = new Shader();
		if (!m_shaderPS->Load(L"SSAOBlur.hlsl", ShaderType::Pixel, "PSMain")) return false;

		// --- Root Signature ---
		D3D12_ROOT_PARAMETER rootParameters[1] = {};
		D3D12_DESCRIPTOR_RANGE range0 = {};
		range0.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range0.NumDescriptors = 1;
		range0.BaseShaderRegister = 0;
		range0.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &range0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC clampSampler = {};
		clampSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		clampSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSampler.ShaderRegister = 0;
		clampSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 1;
		rootSigDesc.pParameters = rootParameters;
		rootSigDesc.NumStaticSamplers = 1;
		rootSigDesc.pStaticSamplers = &clampSampler;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) return false;

		// --- PSO ---
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = m_shaderVS->GetBytecode();
		psoDesc.PS = m_shaderPS->GetBytecode();
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)))) return false;

		return true;
	}

	void SSAOBlurPass::Shutdown()
	{
		SAFE_DELETE(m_shaderVS);
		SAFE_DELETE(m_shaderPS);
		SAFE_DELETE(m_blurredMap);
		m_pso.Reset();
		m_rootSignature.Reset();
	}

	void SSAOBlurPass::Resize(ID3D12Device* device, uint32 width, uint32 height)
	{
		if (m_blurredMap) m_blurredMap->Resize(device, width, height);
	}

	void SSAOBlurPass::Execute(Renderer* renderer, ID3D12GraphicsCommandList* cmd, RenderTarget* ssaoMap)
	{
		if (!m_blurredMap || !cmd || !ssaoMap) return;

		ssaoMap->TransitionToShaderResource(cmd);
		m_blurredMap->TransitionToRenderTarget(cmd);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_blurredMap->GetRTV();
		cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)m_blurredMap->GetWidth(), (float)m_blurredMap->GetHeight(), 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, (long)m_blurredMap->GetWidth(), (long)m_blurredMap->GetHeight() };
		cmd->RSSetViewports(1, &vp);
		cmd->RSSetScissorRects(1, &scissor);

		cmd->SetPipelineState(m_pso.Get());
		cmd->SetGraphicsRootSignature(m_rootSignature.Get());

		renderer->BindRenderTargetSRV(cmd, ssaoMap, 0);

		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->DrawInstanced(3, 1, 0, 0);

		m_blurredMap->TransitionToShaderResource(cmd);
	}
}
