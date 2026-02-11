#include "Renderer.h"
#include "Resources/Texture.h"
#include "Core/Log/Logger.h"

namespace Span
{
	Renderer::Renderer()
	{
		viewMatrix = Matrix4x4::Identity();
		projectionMatrix = Matrix4x4::Identity();
	}

	Renderer::~Renderer()
	{
		Shutdown();
	}

	bool Renderer::Initialize(GraphicsContext* inContext)
	{
		this->context = inContext;
		if (!context)
		{
			SPAN_ERROR("Renderer initialized with null context!");
			return false;
		}

		auto device = context->GetDevice();

		// 同期用フェンスの作成
		if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_waitFence))))
		{
			SPAN_ERROR("Failed to create Wait Fence");
			return false;
		}
		m_waitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!m_waitEvent)
		{
			SPAN_ERROR("Failed to create Wait Event");
			return false;
		}
		m_waitFenceValue = 1;

		// 1. Root Signature (入力レイアウト定義)
		if (!CreateRootSignature()) return false;

		// 2. Shader Load
		vs = new Shader();
		if (!vs->Load(L"Basic.hlsl", ShaderType::Vertex, "VSMain")) return false;

		ps = new Shader();
		if (!ps->Load(L"Basic.hlsl", ShaderType::Pixel, "PSMain")) return false;

		// 3. PSO (描画設定)
		if (!CreatePipelineState()) return false;

		// 4. 定数バッファ (動的書き換え用)
		if (!CreateConstantBuffer()) return false;

		if (!InitializeGridResources()) return false;

		SPAN_LOG("Renderer Initialized Successfully!");
		return true;
	}

	void Renderer::Shutdown()
	{
		// 同期用オブジェクトの開放
		WaitForGPU();

		if (m_waitEvent)
		{
			CloseHandle(m_waitEvent);
			m_waitEvent = nullptr;
		}
		m_waitFence.Reset();

		SAFE_DELETE(vs);
		SAFE_DELETE(ps);

		rootSignature.Reset();
		pipelineState.Reset();
		pipelineStateTransparent.Reset();

		constantBuffer.Reset();

		context = nullptr;

		SAFE_DELETE(m_gridShader);
		SAFE_DELETE(m_gridPlane);
		m_gridPSO.Reset();
		m_gridRootSignature.Reset();
	}

	ID3D12GraphicsCommandList* Renderer::BeginFrame()
	{
		if (!context) return nullptr;

		commandList = context->BeginFrame();

		// 共通設定: RootSignature, PSO, Viewport, Topology
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		commandList->SetPipelineState(pipelineState.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// インデックスのリセット
		constantBufferIndex = 0;

		// シーン定数バッファの更新
		struct SceneCB
		{
			Matrix4x4 view;
			Matrix4x4 proj;
			Vector3 camPos;
			float padding;
		};
		SceneCB sceneData;
		sceneData.view = viewMatrix.Transpose();
		sceneData.proj = projectionMatrix.Transpose();
		sceneData.camPos = cameraPosition;

		memcpy(mappedConstantBuffer + (constantBufferIndex * CB_OBJ_SIZE), &sceneData, sizeof(SceneCB));
		constantBufferIndex++;

		return commandList;
	}

	void Renderer::EndFrame()
	{
		if (!context) return;
		context->EndFrame();
		commandList = nullptr;
	}

	void Renderer::OnResize(uint32 width, uint32 height)
	{
		if (context)
		{
			context->OnResize(width, height);
		}
	}

	void Renderer::DrawMesh(Mesh* mesh, Material* material, const Matrix4x4& worldMatrix)
	{
		if (!mesh || !material || !commandList) return;

		if (constantBufferIndex >= MAX_OBJECTS)
		{
			// 簡易的な上限チェック (本来は動的確保すべき)
			return;
		}

		// 1. Transform更新
		Matrix4x4 mvp = worldMatrix * viewMatrix * projectionMatrix;
		TransformData data;
		data.MVP.FromXM(XMMatrixTranspose(mvp.ToXM()));
		data.World.FromXM(XMMatrixTranspose(worldMatrix.ToXM()));

		UINT8* dest = mappedConstantBuffer + (constantBufferIndex * CB_OBJ_SIZE);
		memcpy(dest, &data, sizeof(TransformData));

		// 2. Material更新
		material->Update();

		// 3. コマンドセット
		commandList->SetPipelineState(material->IsTransparent() ? pipelineStateTransparent.Get() : pipelineState.Get());
		commandList->SetGraphicsRootSignature(rootSignature.Get());

		// スロット0: Transform CBV
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = constantBuffer->GetGPUVirtualAddress();
		cbAddress += constantBufferIndex * CB_OBJ_SIZE;
		commandList->SetGraphicsRootConstantBufferView(0, cbAddress);

		// スロット1: Material CBV
		commandList->SetGraphicsRootConstantBufferView(1, material->GetGPUVirtualAddress());

		// スロット2: Texture SRV
		if (material->GetTexture())
		{
			ID3D12DescriptorHeap* ppHeaps[] = { material->GetTexture()->GetSRVHeap() };
			commandList->SetDescriptorHeaps(1, ppHeaps);
			commandList->SetGraphicsRootDescriptorTable(2, material->GetTexture()->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		}

		mesh->Draw(commandList);

		constantBufferIndex++;
	}

	void Renderer::SetCamera(const Matrix4x4& view, const Matrix4x4 projection)
	{
		viewMatrix = view;
		projectionMatrix = projection;
	}

	void Renderer::WaitForGPU()
	{
		if (!context || !context->GetCommandQueue() || !m_waitFence) return;

		ID3D12CommandQueue* queue = context->GetCommandQueue();

		// 次の値をシグナル
		const uint64_t fenceToWaitFor = m_waitFenceValue;
		queue->Signal(m_waitFence.Get(), fenceToWaitFor);
		m_waitFenceValue++;

		// GPUがここまで到達するのを待つ
		if (m_waitFence->GetCompletedValue() < fenceToWaitFor)
		{
			m_waitFence->SetEventOnCompletion(fenceToWaitFor, m_waitEvent);
			WaitForSingleObject(m_waitEvent, INFINITE);
		}
	}

	bool Renderer::CreateRootSignature()
	{
		// ルートパラメータ定義 (CBV x2, Table x1)
		D3D12_ROOT_PARAMETER rootParameters[3];

		// [0] Transform (b0)
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor.ShaderRegister = 0;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		// [1] Material (b1)
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].Descriptor.ShaderRegister = 1;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// [2] Texture Table (t0)
		D3D12_DESCRIPTOR_RANGE descriptorRange = {};
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange.NumDescriptors = 1;
		descriptorRange.BaseShaderRegister = 0;
		descriptorRange.RegisterSpace = 0;
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// サンプラー (Static Sampler)
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = rootParameters;
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &sampler;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)))
		{
			SPAN_ERROR("RootSignature Serialize Failed: %s", (char*)error->GetBufferPointer());
			return false;
		}

		if (FAILED(context->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
		{
			return false;
		}

		return true;
	}

	bool Renderer::CreatePipelineState()
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS = vs->GetBytecode();
		psoDesc.PS = ps->GetBytecode();
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		// 1. Opaque PSO
		D3D12_RENDER_TARGET_BLEND_DESC opaqueBlend = {};
		opaqueBlend.BlendEnable = FALSE;
		opaqueBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[0] = opaqueBlend;

		if (FAILED(context->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState))))
		{
			SPAN_ERROR("Failed to create Opaque PSO");
			return false;
		}

		// 2. Transparent PSO
		D3D12_RENDER_TARGET_BLEND_DESC transBlend = {};
		transBlend.BlendEnable = TRUE;
		transBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		transBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		transBlend.BlendOp = D3D12_BLEND_OP_ADD;
		transBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
		transBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
		transBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		transBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[0] = transBlend;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 深度書き込みなし

		if (FAILED(context->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateTransparent))))
		{
			SPAN_ERROR("Failed to create Transparent PSO");
			return false;
		}

		return true;
	}

	bool Renderer::CreateConstantBuffer()
	{
		uint32 bufferSize = CB_OBJ_SIZE * MAX_OBJECTS;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = bufferSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		if (FAILED(context->GetDevice()->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&constantBuffer))))
		{
			return false;
		}

		D3D12_RANGE readRange = { 0, 0 };
		constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedConstantBuffer));
		return true;
	}

	bool Renderer::InitializeGridResources()
	{
		auto device = context->GetDevice();

		// 1. シェーダー読み込み
		// VSとPSを個別にロードして保持
		Shader* gridVS = new Shader();
		if (!gridVS->Load(L"EditorGrid.hlsl", ShaderType::Vertex, "VSMain"))
		{
			SPAN_ERROR("Failed to load EditorGrid VS");
			return false;
		}

		Shader* gridPS = new Shader();
		if (!gridPS->Load(L"EditorGrid.hlsl", ShaderType::Pixel, "PSMain"))
		{
			SPAN_ERROR("Failed to load EditorGrid PS");
			delete gridVS;
			return false;
		}

		// メンバ変数の m_gridShader には代表して VS を入れておく（終了処理でdeleteするため）
		// 本来は配列で管理するか、それぞれメンバを持つべきですが、今回は簡易対応
		m_gridShader = gridVS;

		// 2. Root Signature (カメラCBVのみ)
		D3D12_ROOT_PARAMETER slotRootParameter[1];
		slotRootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		slotRootParameter[0].Descriptor.ShaderRegister = 0; // b0
		slotRootParameter[0].Descriptor.RegisterSpace = 0;
		slotRootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 1;
		rootSigDesc.pParameters = slotRootParameter;
		rootSigDesc.NumStaticSamplers = 0;
		rootSigDesc.pStaticSamplers = nullptr;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)))
			return false;

		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_gridRootSignature))))
			return false;

		// 3. PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_gridRootSignature.Get();

		// Shaderクラスのメソッドに合わせて修正
		psoDesc.VS = gridVS->GetBytecode();
		psoDesc.PS = gridPS->GetBytecode();

		// Blend State
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState = blendDesc;

		// Rasterizer State
		D3D12_RASTERIZER_DESC rasterDesc = {};
		rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterDesc.CullMode = D3D12_CULL_MODE_NONE; // 両面描画
		rasterDesc.FrontCounterClockwise = FALSE;
		rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterDesc.DepthClipEnable = TRUE;
		rasterDesc.MultisampleEnable = FALSE;
		rasterDesc.AntialiasedLineEnable = FALSE;
		rasterDesc.ForcedSampleCount = 0;
		rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		psoDesc.RasterizerState = rasterDesc;

		// Depth Stencil State
		// ★ 修正: 型名を D3D12_DEPTH_STENCIL_DESC に変更
		D3D12_DEPTH_STENCIL_DESC depthDesc = {};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込みなし (半透明)
		depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = depthDesc;

		// Input Layout
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };

		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleMask = UINT_MAX;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_gridPSO))))
		{
			SPAN_ERROR("Failed to create Grid PSO");
			delete gridPS;
			return false;
		}

		delete gridPS; // PSO作成後は不要

		// 4. 平面メッシュ作成 (巨大な板)
		float size = 2000.0f;

		// Y軸の太さと高さ
		float yW = 0.05f;  // 幅
		float yH = 1000.0f; // 高さ

		std::vector<Vertex> vertices = {
			// --- 床面 (Normal = 0,1,0) ---
			{ { -size, 0.0f,  size }, { 0,1,0 }, { 0.0f, 1.0f } },
			{ {	 size, 0.0f,  size }, { 0,1,0 }, { 1.0f, 1.0f } },
			{ { -size, 0.0f, -size }, { 0,1,0 }, { 0.0f, 0.0f } },
			{ { -size, 0.0f, -size }, { 0,1,0 }, { 0.0f, 0.0f } },
			{ {	 size, 0.0f,  size }, { 0,1,0 }, { 1.0f, 1.0f } },
			{ {	 size, 0.0f, -size }, { 0,1,0 }, { 1.0f, 0.0f } },

			// --- Y軸 (Normal = 1,0,0) ---
			// 十字にクロスした板を作成して、どの角度からも見えるようにする
			// 1枚目 (Z方向に向いた板)
			{ { -yW, 0.0f, 0.0f }, { 1,0,0 }, { 0,0 } },
			{ {	 yW, 0.0f, 0.0f }, { 1,0,0 }, { 1,0 } },
			{ { -yW,  yH, 0.0f }, { 1,0,0 }, { 0,1 } },
			{ { -yW,  yH, 0.0f }, { 1,0,0 }, { 0,1 } },
			{ {	 yW, 0.0f, 0.0f }, { 1,0,0 }, { 1,0 } },
			{ {	 yW,  yH, 0.0f }, { 1,0,0 }, { 1,1 } },

			// 2枚目 (X方向に向いた板)
			{ { 0.0f, 0.0f, -yW }, { 1,0,0 }, { 0,0 } },
			{ { 0.0f, 0.0f,	 yW }, { 1,0,0 }, { 1,0 } },
			{ { 0.0f,  yH, -yW }, { 1,0,0 }, { 0,1 } },
			{ { 0.0f,  yH, -yW }, { 1,0,0 }, { 0,1 } },
			{ { 0.0f, 0.0f,	 yW }, { 1,0,0 }, { 1,0 } },
			{ { 0.0f,  yH,	yW }, { 1,0,0 }, { 1,1 } },
		};

		m_gridPlane = new Mesh();
		// ★ 修正: インデックスなしの2引数版 Initialize を使用
		m_gridPlane->Initialize(device, vertices);

		return true;
	}

	void Renderer::RenderGrid(ID3D12GraphicsCommandList* cmd)
	{
		if (!m_gridPSO || !m_gridPlane) return;

		cmd->SetPipelineState(m_gridPSO.Get());
		cmd->SetGraphicsRootSignature(m_gridRootSignature.Get());

		// ★ 修正: SetDescriptorHeaps は不要 (CBVはルートパラメータで直接指定するため)
		// ID3D12DescriptorHeap* heaps[] = { constantBuffer.Get() };
		// cmd->SetDescriptorHeaps(1, heaps);

		// 定数バッファのアドレスを設定
		// 注意: constantBufferIndexはBeginFrameで進んでいるので、ここでは先頭(0番目)のアドレスを使用する
		// シーン全体用の定数はバッファの先頭にあると仮定
		cmd->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());

		m_gridPlane->Draw(cmd);
	}
}
