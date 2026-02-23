/*****************************************************************//**
 * @file	ShadowPass.cpp
 * @brief	ShadowPassの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "ShadowPass.h"

namespace Span
{
	bool ShadowPass::Initialize(ID3D12Device* device, uint32 width, uint32 height, uint32 arraySize, bool isCube)
	{
		m_shadowMap = new ShadowMap();
		if (!m_shadowMap->Initialize(device, width, height, arraySize, isCube)) return false;

		m_shaderVS = new Shader();
		if (!m_shaderVS->Load(L"Shadow.hlsl", ShaderType::Vertex, "VSMain")) return false;

		D3D12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor.ShaderRegister = 0;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 1;
		rootSigDesc.pParameters = rootParameters;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) return false;

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = m_shaderVS->GetBytecode();
		psoDesc.PS = { nullptr, 0 }; // 深度のみ書き込むのでPS不要

		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.RasterizerState.DepthBias = 0;
		psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
		psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 0;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)))) return false;
		return true;
	}

	void ShadowPass::Shutdown()
	{
		SAFE_DELETE(m_shaderVS);
		SAFE_DELETE(m_shadowMap);
		m_pso.Reset();
		m_rootSignature.Reset();
	}

	void ShadowPass::BeginPass(ID3D12GraphicsCommandList* cmd)
	{
		if (!m_shadowMap || !cmd) return;
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_shadowMap->GetResource();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		cmd->ResourceBarrier(1, &barrier);

		cmd->SetPipelineState(m_pso.Get());
		cmd->SetGraphicsRootSignature(m_rootSignature.Get());
	}

	void ShadowPass::SetRenderTarget(ID3D12GraphicsCommandList* cmd, uint32 sliceIndex)
	{
		if (!m_shadowMap || !cmd) return;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_shadowMap->GetDSV(sliceIndex);
		cmd->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
		cmd->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)m_shadowMap->GetWidth(), (float)m_shadowMap->GetHeight(), 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, (long)m_shadowMap->GetWidth(), (long)m_shadowMap->GetHeight() };
		cmd->RSSetViewports(1, &vp);
		cmd->RSSetScissorRects(1, &scissor);
	}

	void ShadowPass::EndPass(ID3D12GraphicsCommandList* cmd)
	{
		if (!m_shadowMap || !cmd) return;
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_shadowMap->GetResource();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		cmd->ResourceBarrier(1, &barrier);
	}

	void ShadowPass::DrawMesh(Renderer* renderer, ID3D12GraphicsCommandList* cmd, Mesh* mesh, const Matrix4x4& worldMatrix, const Matrix4x4& lightSpaceMatrix)
	{
		if (!mesh || !cmd) return;

		TransformData data;
		Matrix4x4 mvp = worldMatrix * lightSpaceMatrix;
		data.MVP.FromXM(XMMatrixTranspose(mvp.ToXM()));
		data.World.FromXM(XMMatrixTranspose(worldMatrix.ToXM()));

		D3D12_GPU_VIRTUAL_ADDRESS cbAddr = renderer->AllocateCBV(&data, sizeof(TransformData));
		if (cbAddr == 0) return;

		cmd->SetGraphicsRootConstantBufferView(0, cbAddr);
		mesh->Draw(cmd);
	}
}
