#include "Renderer.h"
#include "Resources/Texture.h"
#include "Core/Log/Logger.h"
#include "Graphics/Core/IBLBuilder.h"

// Passのインクルード
#include "Passes/GridPass.h"
#include "Passes/SkyboxPass.h"
#include "Passes/ShadowPass.h"

namespace Span
{
	Renderer::Renderer()
	{
		viewMatrix = Matrix4x4::Identity();
		projectionMatrix = Matrix4x4::Identity();
	}

	Renderer::~Renderer() { Shutdown(); }

	bool Renderer::Initialize(GraphicsContext* inContext)
	{
		this->context = inContext;
		if (!context) return false;
		auto device = context->GetDevice();

		if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_waitFence)))) return false;
		m_waitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		m_waitFenceValue = 1;

		if (!CreateRootSignature()) return false;

		vs = new Shader(); if (!vs->Load(L"Basic.hlsl", ShaderType::Vertex, "VSMain")) return false;
		ps = new Shader(); if (!ps->Load(L"Basic.hlsl", ShaderType::Pixel, "PSMain")) return false;

		if (!CreatePipelineState()) return false;
		if (!CreateConstantBuffer()) return false;

		// フレーム用の巨大な Descriptor Heap を作成
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 4096;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_frameSrvHeap)))) return false;
		m_srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// --- Passes Initialization ---
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

		m_LightBuffer = new ConstantBuffer<GlobalLightData>();
		if (!m_LightBuffer->Initialize(device)) return false;

		SPAN_LOG("Renderer Initialized Successfully with Render Passes!");
		return true;
	}

	void Renderer::Shutdown()
	{
		WaitForGPU();
		if (m_waitEvent) { CloseHandle(m_waitEvent); m_waitEvent = nullptr; }
		m_waitFence.Reset();

		SAFE_DELETE(vs);
		SAFE_DELETE(ps);
		rootSignature.Reset();
		pipelineState.Reset();
		pipelineStateTransparent.Reset();
		constantBuffer.Reset();

		m_gridPass.reset();
		m_skyboxPass.reset();
		m_dirShadowPass.reset();
		m_spotShadowPass.reset();

		if (m_LightBuffer) { m_LightBuffer->Shutdown(); SAFE_DELETE(m_LightBuffer); }
		context = nullptr;
	}

	D3D12_GPU_VIRTUAL_ADDRESS Renderer::AllocateCBV(const void* data, size_t sizeInBytes)
	{
		if (constantBufferIndex >= MAX_OBJECTS) return 0;

		UINT8* dest = mappedConstantBuffer + (constantBufferIndex * CB_OBJ_SIZE);
		memcpy(dest, data, sizeInBytes);

		D3D12_GPU_VIRTUAL_ADDRESS addr = constantBuffer->GetGPUVirtualAddress() + (constantBufferIndex * CB_OBJ_SIZE);
		constantBufferIndex++;

		return addr;
	}

	ID3D12GraphicsCommandList* Renderer::BeginFrame()
	{
		if (!context) return nullptr;
		commandList = context->BeginFrame();

		commandList->SetGraphicsRootSignature(rootSignature.Get());
		commandList->SetPipelineState(pipelineState.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// フレームの最初にカタログの書き込み位置をリセットし、GPUにセット
		m_frameSrvHeapOffset = 0;
		ID3D12DescriptorHeap* heaps[] = { m_frameSrvHeap.Get() };
		commandList->SetDescriptorHeaps(1, heaps);

		constantBufferIndex = 0;

		struct SceneCB { Matrix4x4 view; Matrix4x4 proj; Vector3 camPos; float pad; };
		SceneCB sceneData = { viewMatrix.Transpose(), projectionMatrix.Transpose(), cameraPosition, 0.0f };

		AllocateCBV(&sceneData, sizeof(SceneCB));

		return commandList;
	}

	void Renderer::EndFrame()
	{
		if (!context) return;
		context->EndFrame();
		commandList = nullptr;
	}

	void Renderer::OnResize(uint32 width, uint32 height) { if (context) context->OnResize(width, height); }

	void Renderer::DrawMesh(Mesh* mesh, Material* material, const Matrix4x4& worldMatrix)
	{
		if (!mesh || !material || !commandList) return;
		auto device = context->GetDevice();

		Matrix4x4 mvp = worldMatrix * viewMatrix * projectionMatrix;
		TransformData data;
		data.MVP.FromXM(XMMatrixTranspose(mvp.ToXM()));
		data.World.FromXM(XMMatrixTranspose(worldMatrix.ToXM()));

		D3D12_GPU_VIRTUAL_ADDRESS cbAddr = AllocateCBV(&data, sizeof(TransformData));
		if (cbAddr == 0) return;

		material->Update();
		commandList->SetPipelineState(material->GetBlendMode() == BlendMode::Transparent ? pipelineStateTransparent.Get() : pipelineState.Get());
		commandList->SetGraphicsRootSignature(rootSignature.Get());

		commandList->SetGraphicsRootConstantBufferView(0, cbAddr);
		commandList->SetGraphicsRootConstantBufferView(1, material->GetGPUVirtualAddress());

		if (m_LightBuffer) commandList->SetGraphicsRootConstantBufferView(14, m_LightBuffer->GetGPUVirtualAddress());

		// PBR Textures (t0 ~ t5)
		Texture* textures[6] = { material->GetAlbedoMap(), material->GetNormalMap(), material->GetMetallicMap(), material->GetRoughnessMap(), material->GetAOMap(), material->GetEmissiveMap() };
		for (int i = 0; i < 6; i++) {
			BindTexture(commandList, textures[i], 2 + i, D3D12_SRV_DIMENSION_TEXTURE2D);
		}

		// Shadows (t6 ~ t8)
		BindShadowMap(commandList, m_dirShadowPass ? m_dirShadowPass->GetShadowMap() : nullptr, 8, D3D12_SRV_DIMENSION_TEXTURE2D);
		BindShadowMap(commandList, m_spotShadowPass ? m_spotShadowPass->GetShadowMap() : nullptr, 9, D3D12_SRV_DIMENSION_TEXTURE2DARRAY);
		BindShadowMap(commandList, m_pointShadowPass ? m_pointShadowPass->GetShadowMap() : nullptr, 10, D3D12_SRV_DIMENSION_TEXTURECUBE);

		// IBL Textures (t9 ~ t11)
		BindTexture(commandList, m_irradianceMap.get(), 11, D3D12_SRV_DIMENSION_TEXTURECUBE);
		BindTexture(commandList, m_prefilterMap.get(),	12, D3D12_SRV_DIMENSION_TEXTURECUBE);
		BindTexture(commandList, m_brdfLUT.get(),		13, D3D12_SRV_DIMENSION_TEXTURE2D);

		mesh->Draw(commandList);
	}

	void Renderer::SetCamera(const Matrix4x4& view, const Matrix4x4 projection)
	{
		viewMatrix = view; projectionMatrix = projection;
		DirectX::XMVECTOR det;
		DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&det, view.ToXM());
		cameraPosition = Vector3(DirectX::XMVectorGetX(invView.r[3]), DirectX::XMVectorGetY(invView.r[3]), DirectX::XMVectorGetZ(invView.r[3]));
	}

	void Renderer::SetGlobalLightData(const std::vector<LightDataGPU>& lights, const EnvironmentSettings& env)
	{
		m_CurrentLightData.CameraPosition = cameraPosition;
		m_CurrentLightData.Exposure = env.Exposure;
		m_CurrentLightData.AmbientIntensity = env.AmbientIntensity;
		m_CurrentLightData.EnvReflectionIntensity = env.EnvReflectionIntensity;
		m_CurrentLightData.SkyTopColor = env.SkyTopColor;
		m_CurrentLightData.SkyHorizonColor = env.SkyHorizonColor;
		m_CurrentLightData.SkyBottomColor = env.SkyBottomColor;
		m_CurrentLightData.SkyMode = (m_envCubemap != nullptr && env.Mode == SkyboxMode::HDRI) ? 1 : 0;

		// 行列の初期化
		m_CurrentLightData.DirectionalLightSpaceMatrix = Matrix4x4::Identity().Transpose();

		m_CurrentLightData.ActiveLightCount = std::min((int)lights.size(), MAX_LIGHTS);
		for (int i = 0; i < m_CurrentLightData.ActiveLightCount; ++i)
		{
			m_CurrentLightData.Lights[i] = lights[i];

			if (lights[i].Type == 0)
			{
				m_CurrentLightData.DirectionalLightSpaceMatrix = lights[i].ShadowMatrix.Transpose();
			}
		}

		if (m_LightBuffer) m_LightBuffer->Update(m_CurrentLightData);
	}

	bool Renderer::LoadEnvironmentMap(const std::string& filepath)
	{
		if (filepath.empty() || m_currentLoadedHDRI == filepath) return true;

		auto device = context->GetDevice();
		auto queue = context->GetCommandQueue();

		// 1. パノラマHDRI画像を読み込む (.hdr)
		Texture panoramaTex;
		if (!panoramaTex.Initialize(device, queue, filepath))
		{
			SPAN_ERROR("Failed to load HDRI: %s", filepath.c_str());
			return false;
		}

		// 2. 出力先の空の Cubemap を作成
		m_envCubemap = std::make_unique<Texture>();
		if (!m_envCubemap->InitializeAsCubemap(device, 1024)) return false;

		// 3. Builderの初期化
		IBLBuilder builder;
		if (!builder.Initialize(device)) return false;

		// 4. Compute Shader用の1回限りのコマンドリストを作成
		ComPtr<ID3D12CommandAllocator> cmdAlloc;
		ComPtr<ID3D12GraphicsCommandList> cmdList;
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));

		// 5. 変換処理
		builder.GenerateCubemapFromPanorama(device, cmdList.Get(), panoramaTex.GetCPUDescriptorHandle(), m_envCubemap.get(), 1024);

		// m_envCubemap のステートを UAV から SRV に遷移
		D3D12_RESOURCE_BARRIER envBarrier = {};
		envBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		envBarrier.Transition.pResource = m_envCubemap->GetResource();
		envBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		envBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		envBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		cmdList->ResourceBarrier(1, &envBarrier);

		// 出力先のテクスチャを作成
		m_irradianceMap = std::make_unique<Texture>();
		m_irradianceMap->InitializeAsCubemap(device, 32);

		m_prefilterMap = std::make_unique<Texture>();
		m_prefilterMap->InitializeAsCubemap(device, 128, 5);

		m_brdfLUT = std::make_unique<Texture>();
		m_brdfLUT->InitializeAsTexture2D(device, 512, 512, DXGI_FORMAT_R16G16_FLOAT);

		// 事前計算の実行
		builder.GenerateIrradianceMap(device, cmdList.Get(), m_envCubemap->GetCPUDescriptorHandle(), m_irradianceMap.get(), 32);
		builder.GeneratePrefilterMap(device, cmdList.Get(), m_envCubemap->GetCPUDescriptorHandle(), m_prefilterMap.get(), 128);
		builder.GenerateBRDFLUT(device, cmdList.Get(), m_brdfLUT.get(), 512);

		// 生成した3つのIBLテクスチャのステートを UAV から SRV に遷移
		D3D12_RESOURCE_BARRIER iblBarriers[3] = {};
		// Irradiance
		iblBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		iblBarriers[0].Transition.pResource = m_irradianceMap->GetResource();
		iblBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		iblBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		iblBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		// Prefilter
		iblBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		iblBarriers[1].Transition.pResource = m_prefilterMap->GetResource();
		iblBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		iblBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		iblBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		// BRDF LUT
		iblBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		iblBarriers[2].Transition.pResource = m_brdfLUT->GetResource();
		iblBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		iblBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		iblBarriers[2].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		cmdList->ResourceBarrier(3, iblBarriers);

		// 6. コマンドリストを閉じて実行
		cmdList->Close();
		ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
		queue->ExecuteCommandLists(1, ppCommandLists);

		// GPUの完了を待機
		ComPtr<ID3D12Fence> fence;
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		queue->Signal(fence.Get(), 1);

		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fence->GetCompletedValue() < 1)
		{
			fence->SetEventOnCompletion(1, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		CloseHandle(fenceEvent);

		builder.Shutdown();

		m_currentLoadedHDRI = filepath;
		SPAN_LOG("Environment Cubemap generated successfully from %s", filepath.c_str());
		return true;
	}

	void Renderer::BindTexture(ID3D12GraphicsCommandList* cmd, Texture* texture, uint32 rootIndex, D3D12_SRV_DIMENSION dimension)
	{
		if (!m_frameSrvHeap) return;

		auto device = context->GetDevice();
		D3D12_CPU_DESCRIPTOR_HANDLE destCpu = m_frameSrvHeap->GetCPUDescriptorHandleForHeapStart();
		destCpu.ptr += m_frameSrvHeapOffset * m_srvDescriptorSize;

		D3D12_GPU_DESCRIPTOR_HANDLE destGpu = m_frameSrvHeap->GetGPUDescriptorHandleForHeapStart();
		destGpu.ptr += m_frameSrvHeapOffset * m_srvDescriptorSize;

		if (texture && texture->GetSRVHeap())
		{
			// テクスチャが存在する場合は通常通りコピー
			device->CopyDescriptorsSimple(1, destCpu, texture->GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else
		{
			// テクスチャが無い場合
			D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
			nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			nullDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			nullDesc.ViewDimension = dimension;
			if (dimension == D3D12_SRV_DIMENSION_TEXTURECUBE) nullDesc.TextureCube.MipLevels = 1;
			else if (dimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY) { nullDesc.Texture2DArray.MipLevels = 1; nullDesc.Texture2DArray.ArraySize = 1; }
			else nullDesc.Texture2D.MipLevels = 1;

			device->CreateShaderResourceView(nullptr, &nullDesc, destCpu);
		}

		cmd->SetGraphicsRootDescriptorTable(rootIndex, destGpu);
		m_frameSrvHeapOffset++;
	}

	void Renderer::BindShadowMap(ID3D12GraphicsCommandList* cmd, ShadowMap* shadowMap, uint32 rootIndex, D3D12_SRV_DIMENSION dimension)
	{
		if (!m_frameSrvHeap) return;

		auto device = context->GetDevice();
		D3D12_CPU_DESCRIPTOR_HANDLE destCpu = m_frameSrvHeap->GetCPUDescriptorHandleForHeapStart();
		destCpu.ptr += m_frameSrvHeapOffset * m_srvDescriptorSize;

		D3D12_GPU_DESCRIPTOR_HANDLE destGpu = m_frameSrvHeap->GetGPUDescriptorHandleForHeapStart();
		destGpu.ptr += m_frameSrvHeapOffset * m_srvDescriptorSize;

		if (shadowMap && shadowMap->GetSRVHeap())
		{
			device->CopyDescriptorsSimple(1, destCpu, shadowMap->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else
		{
			// 影用の安全なダミーSRVを生成
			D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
			nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			nullDesc.Format = DXGI_FORMAT_R32_FLOAT;
			nullDesc.ViewDimension = dimension;
			if (dimension == D3D12_SRV_DIMENSION_TEXTURECUBE) nullDesc.TextureCube.MipLevels = 1;
			else if (dimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY) { nullDesc.Texture2DArray.MipLevels = 1; nullDesc.Texture2DArray.ArraySize = 6; }
			else nullDesc.Texture2D.MipLevels = 1;

			device->CreateShaderResourceView(nullptr, &nullDesc, destCpu);
		}

		cmd->SetGraphicsRootDescriptorTable(rootIndex, destGpu);
		m_frameSrvHeapOffset++;
	}

	void Renderer::WaitForGPU()
	{
		if (!context || !context->GetCommandQueue() || !m_waitFence) return;
		ID3D12CommandQueue* queue = context->GetCommandQueue();
		const uint64_t fenceToWaitFor = m_waitFenceValue;
		queue->Signal(m_waitFence.Get(), fenceToWaitFor);
		m_waitFenceValue++;
		if (m_waitFence->GetCompletedValue() < fenceToWaitFor) {
			m_waitFence->SetEventOnCompletion(fenceToWaitFor, m_waitEvent);
			WaitForSingleObject(m_waitEvent, INFINITE);
		}
	}

	bool Renderer::CreateRootSignature()
	{
		D3D12_ROOT_PARAMETER rootParameters[15];

		// [0] Transform (b0)
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor.ShaderRegister = 0;
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		// [1] Material (b1)
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].Descriptor.ShaderRegister = 1;
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// [2] ~ [9] Texture Tables (t0 ~ t7) : PBR(6) + DirShadow(1) + SpotShadowArray(1)
		D3D12_DESCRIPTOR_RANGE ranges[12] = {};
		for (int i = 0; i < 12; i++)
		{
			ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			ranges[i].NumDescriptors = 1;
			ranges[i].BaseShaderRegister = i;
			ranges[i].RegisterSpace = 0;
			ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			rootParameters[2 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[2 + i].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[2 + i].DescriptorTable.pDescriptorRanges = &ranges[i];
			rootParameters[2 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		}

		// [10] LightBuffer (b2)
		rootParameters[14].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[14].Descriptor.ShaderRegister = 2;
		rootParameters[14].Descriptor.RegisterSpace = 0;
		rootParameters[14].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// --- Samplers ---
		// s0: 通常のテクスチャサンプラー
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_ANISOTROPIC;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 8;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// s1: 影判定用の比較サンプラー (PCF用)
		D3D12_STATIC_SAMPLER_DESC shadowSampler = {};
		shadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		shadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		shadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		shadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		shadowSampler.ShaderRegister = 1;
		shadowSampler.RegisterSpace = 0;
		shadowSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC samplers[] = { sampler, shadowSampler };

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = rootParameters;
		rootSignatureDesc.NumStaticSamplers = _countof(samplers);
		rootSignatureDesc.pStaticSamplers = samplers;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature, error;
		if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;
		if (FAILED(context->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))) return false;

		return true;
	}

	bool Renderer::CreatePipelineState()
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS = vs->GetBytecode();
		psoDesc.PS = ps->GetBytecode();

		// Rasterizer State
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;

		// Depth Stencil State
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		// 1. Opaque PSO (不透明)
		D3D12_RENDER_TARGET_BLEND_DESC opaqueBlend = {};
		opaqueBlend.BlendEnable = FALSE;
		opaqueBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[0] = opaqueBlend;
		if (FAILED(context->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)))) return false;

		// 2. Transparent PSO (半透明)
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
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 半透明は深度を書き込まない

		if (FAILED(context->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateTransparent)))) return false;

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
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer))))
		{
			return false;
		}

		D3D12_RANGE readRange = { 0, 0 };
		constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedConstantBuffer));

		return true;
	}
}
