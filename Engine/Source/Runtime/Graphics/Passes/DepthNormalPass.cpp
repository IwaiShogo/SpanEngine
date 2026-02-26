/*****************************************************************//**
 * @file	DepthNormalPass.cpp
 * @brief	DepthNormalPassの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "DepthNormalPass.h"

namespace Span
{
	bool DepthNormalPass::Initialize(ID3D12Device* device, uint32 width, uint32 height)
	{
		m_gBuffer = new RenderTarget();

		if (!m_gBuffer->Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT)) return false;

		m_shaderVS = new Shader();
		if (!m_shaderVS->Load(L"DepthNormal.hlsl", ShaderType::Vertex, "VSMain")) return false;

		m_shaderPS = new Shader();
		if (!m_shaderPS->Load(L"DepthNormal.hlsl", ShaderType::Pixel, "PSMain")) return false;

		// --- Root Signature ---
		D3D12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor.ShaderRegister = 0; // b0
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 1;
		rootSigDesc.pParameters = rootParameters;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) return false;

		// --- Pipeline State ---
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = m_shaderVS->GetBytecode();
		psoDesc.PS = m_shaderPS->GetBytecode();

		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;

		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		D3D12_RENDER_TARGET_BLEND_DESC defaultBlend = {};
		defaultBlend.BlendEnable = FALSE;
		defaultBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[0] = defaultBlend;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)))) return false;

		return true;
	}

	void DepthNormalPass::Shutdown()
	{
		SAFE_DELETE(m_shaderVS);
		SAFE_DELETE(m_shaderPS);
		SAFE_DELETE(m_gBuffer);
		m_pso.Reset();
		m_rootSignature.Reset();
	}

	void DepthNormalPass::Resize(ID3D12Device* device, uint32 width, uint32 height)
	{
		if (m_gBuffer)
		{
			m_gBuffer->Resize(device, width, height);
		}
	}

	void DepthNormalPass::BeginPass(ID3D12GraphicsCommandList* cmd)
	{
		if (!m_gBuffer || !cmd) return;

		m_gBuffer->TransitionToRenderTarget(cmd);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_gBuffer->GetRTV();
		D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_gBuffer->GetDSV();
		cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		// 法線(0,0,0)、深度(遠方:10000.0等)としてクリア
		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 10000.0f };
		cmd->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)m_gBuffer->GetWidth(), (float)m_gBuffer->GetHeight(), 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, (long)m_gBuffer->GetWidth(), (long)m_gBuffer->GetHeight() };
		cmd->RSSetViewports(1, &vp);
		cmd->RSSetScissorRects(1, &scissor);

		cmd->SetPipelineState(m_pso.Get());
		cmd->SetGraphicsRootSignature(m_rootSignature.Get());
	}

	void DepthNormalPass::EndPass(ID3D12GraphicsCommandList* cmd)
	{
		if (!m_gBuffer || !cmd) return;
		m_gBuffer->TransitionToShaderResource(cmd);
	}

	void DepthNormalPass::DrawMesh(Renderer* renderer, ID3D12GraphicsCommandList* cmd, Mesh* mesh, const Matrix4x4& worldMatrix, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix)
	{
		if (!mesh || !cmd || !renderer) return;

		DepthNormalData data;
		Matrix4x4 mvp = worldMatrix * viewMatrix * projectionMatrix;
		data.MVP.FromXM(XMMatrixTranspose(mvp.ToXM()));
		data.World.FromXM(XMMatrixTranspose(worldMatrix.ToXM()));
		data.View.FromXM(XMMatrixTranspose(viewMatrix.ToXM()));

		D3D12_GPU_VIRTUAL_ADDRESS cbAddr = renderer->AllocateCBV(&data, sizeof(DepthNormalData));
		if (cbAddr == 0) return;

		cmd->SetGraphicsRootConstantBufferView(0, cbAddr);
		mesh->Draw(cmd);
	}
}
