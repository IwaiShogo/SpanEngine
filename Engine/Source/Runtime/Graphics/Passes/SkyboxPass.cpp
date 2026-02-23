/*****************************************************************//**
 * @file	SkyboxPass.cpp
 * @brief	SkyboxPassの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "SkyboxPass.h"

namespace Span
{
	bool SkyboxPass::Initialize(ID3D12Device* device)
	{
		m_shaderVS = new Shader();
		if (!m_shaderVS->Load(L"Skybox.hlsl", ShaderType::Vertex, "VSMain")) return false;

		m_shaderPS = new Shader();
		if (!m_shaderPS->Load(L"Skybox.hlsl", ShaderType::Pixel, "PSMain")) return false;

		// 1. Root Signature (b0:Camera, b1:Settings, t0:Cubemap)
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 1;
		srvRange.BaseShaderRegister = 0;
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParameters[3];

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor.ShaderRegister = 0;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].Descriptor.ShaderRegister = 1;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// Cubemap用 SRV テーブル
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &srvRange;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// サンプラーの追加 (s0)
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = _countof(rootParameters);
		rootSigDesc.pParameters = rootParameters;
		rootSigDesc.NumStaticSamplers = 1;
		rootSigDesc.pStaticSamplers = &sampler;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) return false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = m_shaderVS->GetBytecode();
		psoDesc.PS = m_shaderPS->GetBytecode();
		psoDesc.InputLayout = { nullptr, 0 }; // Fullscreen quad from SV_VertexID
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // 最奥(1.0)に描画するため
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)))) return false;
		return true;
	}

	void SkyboxPass::Shutdown()
	{
		SAFE_DELETE(m_shaderVS);
		SAFE_DELETE(m_shaderPS);
		m_pso.Reset();
		m_rootSignature.Reset();
	}

	void SkyboxPass::Render(Renderer* renderer, ID3D12GraphicsCommandList* cmd, const EnvironmentSettings& env, const Matrix4x4& view, const Matrix4x4& proj, const Vector3& camPos, Texture* envCubemap)
	{
		if (!m_pso) return;

		struct SkyboxCameraCB { Matrix4x4 InvView; Matrix4x4 InvProj; Vector3 CamPos; float pad; };
		struct SkyboxSettingsCB
		{
			Vector3 TopColor;
			float pad1;
			Vector3 HorizonColor;
			float pad2;
			Vector3 BottomColor;
			int SkyMode;
			float Exposure;
			float pad3[3];
		};

		SkyboxCameraCB camData;
		camData.InvView.FromXM(XMMatrixTranspose(XMMatrixInverse(nullptr, view.ToXM())));
		camData.InvProj.FromXM(XMMatrixTranspose(XMMatrixInverse(nullptr, proj.ToXM())));
		camData.CamPos = camPos;

		SkyboxSettingsCB setData;
		setData.TopColor = env.SkyTopColor;
		setData.HorizonColor = env.SkyHorizonColor;
		setData.BottomColor = env.SkyTopColor;

		setData.SkyMode = (envCubemap != nullptr) ? 1 : 0;
		setData.Exposure = env.Exposure;

		// Rendererの機能を使ってCBVを割り当ててGPUアドレスを取得
		D3D12_GPU_VIRTUAL_ADDRESS cbCamAddress = renderer->AllocateCBV(&camData, sizeof(SkyboxCameraCB));
		D3D12_GPU_VIRTUAL_ADDRESS cbSetAddress = renderer->AllocateCBV(&setData, sizeof(SkyboxSettingsCB));

		if (cbCamAddress == 0 || cbSetAddress == 0) return;

		cmd->SetPipelineState(m_pso.Get());
		cmd->SetGraphicsRootSignature(m_rootSignature.Get());
		cmd->SetGraphicsRootConstantBufferView(0, cbCamAddress);
		cmd->SetGraphicsRootConstantBufferView(1, cbSetAddress);

		if (envCubemap && envCubemap->GetSRVHeap())
		{
			ID3D12DescriptorHeap* heaps[] = { envCubemap->GetSRVHeap() };
			cmd->SetDescriptorHeaps(1, heaps);
			cmd->SetGraphicsRootDescriptorTable(2, envCubemap->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		}
		else
		{
			return;
		}

		cmd->IASetVertexBuffers(0, 0, nullptr);
		cmd->IASetIndexBuffer(nullptr);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->DrawInstanced(3, 1, 0, 0);
	}
}
