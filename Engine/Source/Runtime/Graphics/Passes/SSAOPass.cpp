/*****************************************************************//**
 * @file	SSAOPass.cpp
 * @brief	SSAOPassの実装。乱数カーネルとノイズの生成を含む。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "SSAOPass.h"


namespace Span
{
	bool SSAOPass::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, uint32 width, uint32 height)
	{
		m_ssaoMap = new RenderTarget();
		const float whiteClear[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		// 1チャンネル(白黒)の8bitフォーマットで十分
		if (!m_ssaoMap->Initialize(device, width, height, DXGI_FORMAT_R8_UNORM, whiteClear)) return false;

		GenerateSampleKernel();
		GenerateNoiseTexture(device, commandQueue);

		m_shaderVS = new Shader();
		if (!m_shaderVS->Load(L"SSAO.hlsl", ShaderType::Vertex, "VSMain")) return false;

		m_shaderPS = new Shader();
		if (!m_shaderPS->Load(L"SSAO.hlsl", ShaderType::Pixel, "PSMain")) return false;

		// --- Root Signature ---
		D3D12_ROOT_PARAMETER rootParameters[3];
		// [0] SSAO Buffer (b0)
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor.ShaderRegister = 0;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// [1] GBuffer (t0)
		D3D12_DESCRIPTOR_RANGE range0 = {};
		range0.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range0.NumDescriptors = 1;
		range0.BaseShaderRegister = 0;
		range0.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &range0;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// [2] Noise Texture (t1)
		D3D12_DESCRIPTOR_RANGE range1 = {};
		range1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range1.NumDescriptors = 1;
		range1.BaseShaderRegister = 1;
		range1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &range1;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// Samplers
		D3D12_STATIC_SAMPLER_DESC clampSampler = {};
		clampSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // GBufferは補間しない
		clampSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSampler.ShaderRegister = 0;
		clampSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC wrapSampler = {};
		wrapSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		wrapSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // ノイズはリピートさせる
		wrapSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		wrapSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		wrapSampler.ShaderRegister = 1;
		wrapSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC samplers[] = { clampSampler, wrapSampler };

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 3;
		rootSigDesc.pParameters = rootParameters;
		rootSigDesc.NumStaticSamplers = 2;
		rootSigDesc.pStaticSamplers = samplers;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) return false;

		// --- Pipeline State ---
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = m_shaderVS->GetBytecode();
		psoDesc.PS = m_shaderPS->GetBytecode();

		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // フルスクリーン三角形なのでカリングオフ
		psoDesc.RasterizerState.DepthClipEnable = FALSE;

		// 深度判定・書き込みは不要
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)))) return false;

		return true;
	}

	void SSAOPass::Shutdown()
	{
		SAFE_DELETE(m_shaderVS);
		SAFE_DELETE(m_shaderPS);
		SAFE_DELETE(m_ssaoMap);
		m_noiseTexture.reset();
		m_pso.Reset();
		m_rootSignature.Reset();
	}

	void SSAOPass::Resize(ID3D12Device* device, uint32 width, uint32 height)
	{
		if (m_ssaoMap) m_ssaoMap->Resize(device, width, height);
	}

	void SSAOPass::GenerateSampleKernel()
	{
		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		for (int i = 0; i < 64; ++i)
		{
			// X, Yは -1.0 ~ 1.0, Zは 0.0 ~ 1.0 (半球)
			Vector3 sample(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator)
			);
			sample = Vector3::Normalize(sample);
			sample = sample * randomFloats(generator);

			// サンプルを中心近くに集中させる
			float scale = (float)i / 64.0f;
			scale = 0.1f + scale * scale * (1.0f - 0.1f); // Lerp(0.1, 1.0, scale^2)
			sample = sample * scale;

			m_ssaoKernel.push_back({ sample.x, sample.y, sample.z, 0.0f });
		}
	}

	void SSAOPass::GenerateNoiseTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
	{
		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		std::vector<uint32> noiseData(16); // 4x4
		for (int i = 0; i < 16; ++i)
		{
			// Z(B) は 0、XY(RG) にランダムな回転方向を持たせる
			float x = randomFloats(generator) * 2.0f - 1.0f;
			float y = randomFloats(generator) * 2.0f - 1.0f;
			Vector3 noise(x, y, 0.0f);
			noise = Vector3::Normalize(noise);

			// -1~1 を 0~255 の RGBA に変換
			uint8 r = (uint8)((noise.x * 0.5f + 0.5f) * 255.0f);
			uint8 g = (uint8)((noise.y * 0.5f + 0.5f) * 255.0f);
			uint8 b = 0;
			uint8 a = 255;
			noiseData[i] = (a << 24) | (b << 16) | (g << 8) | r;
		}

		m_noiseTexture = std::make_unique<Texture>();
		m_noiseTexture->InitializeFromMemory(device, commandQueue, noiseData.data(), 4, 4, 4);
	}

	void SSAOPass::Execute(Renderer* renderer, ID3D12GraphicsCommandList* cmd, RenderTarget* gBuffer, const Matrix4x4& projectionMatrix)
	{
		if (!m_ssaoMap || !cmd || !gBuffer) return;

		// GBuffer は SRV として読み取る
		gBuffer->TransitionToShaderResource(cmd);
		m_ssaoMap->TransitionToRenderTarget(cmd);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_ssaoMap->GetRTV();
		cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr); // 深度は無し

		const float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // 初期値は白(影なし)
		cmd->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

		D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)m_ssaoMap->GetWidth(), (float)m_ssaoMap->GetHeight(), 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, (long)m_ssaoMap->GetWidth(), (long)m_ssaoMap->GetHeight() };
		cmd->RSSetViewports(1, &vp);
		cmd->RSSetScissorRects(1, &scissor);

		cmd->SetPipelineState(m_pso.Get());
		cmd->SetGraphicsRootSignature(m_rootSignature.Get());

		// CBVのセット
		SSAOData data;
		data.Projection.FromXM(XMMatrixTranspose(projectionMatrix.ToXM()));

		DirectX::XMVECTOR det;
		DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&det, projectionMatrix.ToXM());
		data.InvProjection.FromXM(XMMatrixTranspose(invProj));

		for (int i = 0; i < 64; i++) data.Samples[i] = m_ssaoKernel[i];
		data.NoiseScale = { (float)m_ssaoMap->GetWidth() / 4.0f, (float)m_ssaoMap->GetHeight() / 4.0f };
		data.Radius = 0.5f;
		data.Bias = 0.025f;

		D3D12_GPU_VIRTUAL_ADDRESS cbAddr = renderer->AllocateCBV(&data, sizeof(SSAOData));
		if (cbAddr != 0) cmd->SetGraphicsRootConstantBufferView(0, cbAddr);

		// テクスチャのバインド
		// GBuffer の SRV
		renderer->BindRenderTargetSRV(cmd, gBuffer, 1);

		// Noise の SRV
		renderer->BindTexture(cmd, m_noiseTexture.get(), 2);

		// フルスクリーン三角形描画 (頂点バッファなし)
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->DrawInstanced(3, 1, 0, 0);

		// SSAOマップをSRVに戻す
		m_ssaoMap->TransitionToShaderResource(cmd);
	}
}
