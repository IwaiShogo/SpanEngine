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

		SPAN_LOG("Initializing Renderer (PSO & Resources)...");

		if (!CreateRootSignature()) return false;

		// シェーダーロード
		vs = new Shader();
		if (!vs->Load(L"Basic.hlsl", ShaderType::Vertex, "VSMain")) return false;

		ps = new Shader();
		if (!ps->Load(L"Basic.hlsl", ShaderType::Pixel, "PSMain")) return false;

		if (!CreatePipelineState()) return false;
		if (!CreateConstantBuffer()) return false;

		SPAN_LOG("Renderer Initialized Successfully!");
		return true;
	}

	void Renderer::Shutdown()
	{
		// ContextのGPU待ち合わせはApplication側で行う想定だが、
		// 念のためここでも待てると安全（今回はContext任せにする）

		SAFE_DELETE(vs);
		SAFE_DELETE(ps);

		if (constantBuffer)
		{
			constantBuffer->Unmap(0, nullptr);
			mappedConstantBuffer = nullptr;
			constantBuffer.Reset();
		}
	}

	ID3D12GraphicsCommandList* Renderer::BeginFrame()
	{
		if (!context) return nullptr;

		// Contextに「準備開始」を依頼し、コマンドリストを受け取る
		commandList = context->BeginFrame();
		constantBufferIndex = 0;

		return commandList;
	}

	void Renderer::EndFrame()
	{
		if (!context) return;
		context->EndFrame();
		commandList = nullptr; // 所有権放棄
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
}