/*****************************************************************//**
 * @file	GridPass.cpp
 * @brief	GridPassの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "GridPass.h"

namespace Span
{
	bool GridPass::Initialize(ID3D12Device* device)
	{
		m_shaderVS = new Shader();
		if (!m_shaderVS->Load(L"EditorGrid.hlsl", ShaderType::Vertex, "VSMain")) return false;

		m_shaderPS = new Shader();
		if (!m_shaderPS->Load(L"EditorGrid.hlsl", ShaderType::Pixel, "PSMain")) return false;

		// Root Signature (カメラCBVのみ)
		D3D12_ROOT_PARAMETER slotRootParameter[1];
		slotRootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		slotRootParameter[0].Descriptor.ShaderRegister = 0; // b0
		slotRootParameter[0].Descriptor.RegisterSpace = 0;
		slotRootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 1;
		rootSigDesc.pParameters = slotRootParameter;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) return false;

		// PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = m_shaderVS->GetBytecode();
		psoDesc.PS = m_shaderPS->GetBytecode();

		// Blend State (Alpha Blending)
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState = blendDesc;

		// Rasterizer & Depth
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

		// Input Layout
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleMask = UINT_MAX;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)))) return false;

		// Plane Mesh Generation
		float size = 2000.0f; float yW = 0.05f; float yH = 1000.0f;
		std::vector<Vertex> vertices = {
			{ { -size, 0.0f,  size }, { 0,1,0 }, { 0.0f, 1.0f } }, { { size, 0.0f,	size }, { 0,1,0 }, { 1.0f, 1.0f } }, { { -size, 0.0f, -size }, { 0,1,0 }, { 0.0f, 0.0f } },
			{ { -size, 0.0f, -size }, { 0,1,0 }, { 0.0f, 0.0f } }, { { size, 0.0f,	size }, { 0,1,0 }, { 1.0f, 1.0f } }, { { size, 0.0f, -size }, { 0,1,0 }, { 1.0f, 0.0f } },
			{ { -yW, 0.0f, 0.0f }, { 1,0,0 }, { 0,0 } }, { { yW, 0.0f, 0.0f }, { 1,0,0 }, { 1,0 } }, { { -yW,  yH, 0.0f }, { 1,0,0 }, { 0,1 } },
			{ { -yW,  yH, 0.0f }, { 1,0,0 }, { 0,1 } }, { { yW, 0.0f, 0.0f }, { 1,0,0 }, { 1,0 } }, { { yW,	 yH, 0.0f }, { 1,0,0 }, { 1,1 } },
			{ { 0.0f, 0.0f, -yW }, { 1,0,0 }, { 0,0 } }, { { 0.0f, 0.0f, yW }, { 1,0,0 }, { 1,0 } }, { { 0.0f,	yH, -yW }, { 1,0,0 }, { 0,1 } },
			{ { 0.0f,  yH, -yW }, { 1,0,0 }, { 0,1 } }, { { 0.0f, 0.0f, yW }, { 1,0,0 }, { 1,0 } }, { { 0.0f,  yH, yW }, { 1,0,0 }, { 1,1 } },
		};
		m_planeMesh = new Mesh();
		m_planeMesh->Initialize(device, vertices);

		return true;
	}

	void GridPass::Shutdown()
	{
		SAFE_DELETE(m_shaderVS);
		SAFE_DELETE(m_shaderPS);
		SAFE_DELETE(m_planeMesh);
		m_pso.Reset();
		m_rootSignature.Reset();
	}

	void GridPass::Render(ID3D12GraphicsCommandList* cmd, D3D12_GPU_VIRTUAL_ADDRESS sceneCbAddress)
	{
		if (!m_pso || !m_planeMesh) return;
		cmd->SetPipelineState(m_pso.Get());
		cmd->SetGraphicsRootSignature(m_rootSignature.Get());
		cmd->SetGraphicsRootConstantBufferView(0, sceneCbAddress);
		m_planeMesh->Draw(cmd);
	}
}
