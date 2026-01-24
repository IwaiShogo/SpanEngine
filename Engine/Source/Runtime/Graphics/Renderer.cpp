#include "Renderer.h"
#include "Graphics/Resources/Material.h"
#include "Graphics/Resources/Texture.h"

namespace Span
{
	Renderer::Renderer()
	{
		// Viewportなどの初期化
		viewport = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
		scissorRect = { 0, 0, 0, 0 };
	}

	Renderer::~Renderer()
	{
		Shutdown();
	}

	bool Renderer::Initialize(Window& window)
	{
		SPAN_LOG("Initializing DirectX 12 Renderer...");

		viewport.Width = static_cast<float>(window.GetWidth());
		viewport.Height = static_cast<float>(window.GetHeight());
		scissorRect.right = static_cast<long>(window.GetWidth());
		scissorRect.bottom = static_cast<long>(window.GetHeight());

		// 深度の範囲を設定
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		LoadPipeline(window);
		
		if (!LoadAssets())
		{
			return false;
		}

		SPAN_LOG("DirectX 12 Initialized Successfully!");
		return true;
	}

	void Renderer::LoadPipeline(Window& window)
	{
		uint32 dxgiFactoryFlags = 0;

#if defined(_DEBUG)
		// デバッグレイヤーの有効化 (開発中は必須)
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif

		// 1. Factoryの作成
		CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

		// 2. Deviceの作成 (GPUの選択)
		// 通常は最初に見つかった高性能GPUを使う
		D3D12CreateDevice(
			nullptr, // default adapter
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		);

		// 3. Command Queueの作成
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

		// 4. Swap Chainの作成
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.Width = window.GetWidth();
		swapChainDesc.Height = window.GetHeight();
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		ComPtr<IDXGISwapChain1> swapChain1;
		factory->CreateSwapChainForHwnd(
			commandQueue.Get(),
			window.GetHandle(),
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1
		);
		swapChain1.As(&swapChain); // SwapChain3に変換
		frameIndex = swapChain->GetCurrentBackBufferIndex();

		// 5. Descriptor Heap (RTV) の作成
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// 6. Render Target View (RTV) の作成
		// バックバッファのメモリ場所を特定して、GPUに教える
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (uint32 i = 0; i < FrameCount; i++)
		{
			swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
			device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}

		// 7. Command Allocator の作成
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

		// 8. Depth Stencil Descriptor Heap の作成
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

		dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		// 9. Depth Stencil Buffer リソースの作成
		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = window.GetWidth();
		depthStencilDesc.Height = window.GetHeight();
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT; // 32bit浮動小数点の深度
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = DXGI_FORMAT_D32_FLOAT;
		optClear.DepthStencil.Depth = 1.0f; // 1.0 (最奥) でクリアする
		optClear.DepthStencil.Stencil = 0;

		// デフォルトヒープ（VRAM）に作成
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optClear,
			IID_PPV_ARGS(&depthBuffer)
		);

		// ビュー (DSV) の作成
		device->CreateDepthStencilView(depthBuffer.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	bool Renderer::LoadAssets()
	{
		// 1. ルートシグネチャ作成
		if (!CreateRootSignature()) return false;

		// 2. シェーダーのロード
		vs = new Shader();
		if (!vs->Load(L"Basic.hlsl", ShaderType::Vertex, "VSMain")) return false;

		ps = new Shader();
		if (!ps->Load(L"Basic.hlsl", ShaderType::Pixel, "PSMain")) return false;

		// 3. パイプラインステート(PSO)作成
		if (!CreatePipelineState()) return false;

		// 4. コマンドリスト作成
		// PSOをセットした状態で作成する
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList));
		commandList->Close();

		// ダイナミック定数バッファの作成
		{
			uint32 bufferSize = CB_OBJ_SIZE * MAX_OBJECTS;

			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Alignment = 0;
			resourceDesc.Width = bufferSize;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			if (FAILED(device->CreateCommittedResource(
				&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&constantBuffer))))
			{
				return false;
			}

			// 永続的にマップしておく
			D3D12_RANGE readRange = { 0, 0 };
			constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedConstantBuffer));
		}

		// 5. 同期オブジェクト作成
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		fenceValue = 1;
		fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		WaitForPreviousFrame();

		return true;
	}

	void Renderer::Shutdown()
	{
		if (fenceEvent == nullptr) return;

		WaitForPreviousFrame();
		CloseHandle(fenceEvent);

		fenceEvent = nullptr;

		SAFE_DELETE(vs);
		SAFE_DELETE(ps);

		if (constantBuffer)
		{
			constantBuffer->Unmap(0, nullptr);
			mappedConstantBuffer = nullptr;
			constantBuffer.Reset();
		}
	}

	void Renderer::BeginFrame()
	{
		constantBufferIndex = 0;

		// コマンド記録の準備
		commandAllocator->Reset();
		commandList->Reset(commandAllocator.Get(), nullptr);

		// バックバッファを「表示用」から「描画先」の状態に遷移させる (Resource Barrier)
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = renderTargets[frameIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);

		// ビューポートとシザーの設定
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		// 画面クリア (コーンフラワーブルー: DirectXの伝統色)
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += frameIndex * rtvDescriptorSize;

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.0f }; // Cornflower Blue
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	void Renderer::EndFrame()
	{
		// 描画が終わったので、バッファを「表示用」に戻す
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = renderTargets[frameIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);

		// コマンド記録終了
		commandList->Close();

		// コマンドリストを実行
		ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// 画面更新 (V-Sync ON)
		swapChain->Present(1, 0);

		// 次のフレームへ同期
		WaitForPreviousFrame();
	}

	void Renderer::DrawMesh(Mesh* mesh, Material* material, const Matrix4x4& worldMatrix)
	{
		if (!mesh || !material) return;

		// 上限チェック
		if (constantBufferIndex >= MAX_OBJECTS)
		{
			SPAN_WARN("Draw Call limit reached!");
			return;
		}

		// 1. Transform更新 (既存コード)
		Matrix4x4 mvp = worldMatrix * viewMatrix * projectionMatrix;
		TransformData data;
		data.MVP.FromXM(XMMatrixTranspose(mvp.ToXM()));
		data.World.FromXM(XMMatrixTranspose(worldMatrix.ToXM()));

		UINT8* dest = mappedConstantBuffer + (constantBufferIndex * CB_OBJ_SIZE);
		memcpy(dest, &data, sizeof(TransformData));

		// 2. Material更新
		material->Update();

		// 3. コマンドセット
		if (material->IsTransparent())
		{
			commandList->SetPipelineState(pipelineStateTransparent.Get());
		}
		else
		{
			commandList->SetPipelineState(pipelineState.Get());
		}

		commandList->SetGraphicsRootSignature(rootSignature.Get());

		// スロット0: Transform
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = constantBuffer->GetGPUVirtualAddress();
		cbAddress += constantBufferIndex * CB_OBJ_SIZE;
		commandList->SetGraphicsRootConstantBufferView(0, cbAddress);

		// スロット1: Material
		commandList->SetGraphicsRootConstantBufferView(1, material->GetGPUVirtualAddress());

		// テクスチャのバインド
		if (material->GetTexture())
		{
			// テクスチャが持っているSRVヒープをセット
			ID3D12DescriptorHeap* ppHeaps[] = { material->GetTexture()->GetSRVHeap() };
			commandList->SetDescriptorHeaps(1, ppHeaps);
			// テーブルハンドルをセット (RootParameter[2])
			commandList->SetGraphicsRootDescriptorTable(2, material->GetTexture()->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		}

		mesh->Draw(commandList.Get());

		constantBufferIndex++;
	}

	void Renderer::SetCamera(const Matrix4x4& view, const Matrix4x4 projection)
	{
		viewMatrix = view;
		projectionMatrix = projection;
	}

	void Renderer::WaitForPreviousFrame()
	{
		// シグナルを発行
		const uint64 fenceVal = fenceValue;
		commandQueue->Signal(fence.Get(), fenceVal);
		fenceValue++;

		// GPUがここまで到達したか確認
		if (fence->GetCompletedValue() < fenceVal)
		{
			fence->SetEventOnCompletion(fenceVal, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		frameIndex = swapChain->GetCurrentBackBufferIndex();
	}

	bool Renderer::CreateRootSignature()
	{
		// 1. ルートパラメータ (3つ)
		D3D12_ROOT_PARAMETER rootParameters[3];

		// [0] CBV: Transform
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].Descriptor.ShaderRegister = 0; // b0
		rootParameters[0].Descriptor.RegisterSpace = 0;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		// [1] CBV: Material
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].Descriptor.ShaderRegister = 1; // b1
		rootParameters[1].Descriptor.RegisterSpace = 0;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// [2] Descriptor Table: Texture (t0)
		D3D12_DESCRIPTOR_RANGE descriptorRange = {};
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange.NumDescriptors = 1;
		descriptorRange.BaseShaderRegister = 0; // t0
		descriptorRange.RegisterSpace = 0;
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// 2. サンプラー
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

		// 3. ルートシグネチャ記述
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
			SPAN_ERROR("Failed to serialize Root Signature: %s", (char*)error->GetBufferPointer());
			return false;
		}

		if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
		{
			SPAN_ERROR("Failed to create Root Signature!");
			return false;
		}

		return true;
	}

	bool Renderer::CreatePipelineState()
	{
		// 頂点レイアウトの定義 (Mesh::Vertex構造体と合わせる)
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// PSOの設定構造体
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = rootSignature.Get();

		psoDesc.VS = vs->GetBytecode();
		psoDesc.PS = ps->GetBytecode();

		// ラスタライザ設定 (カリングなし＝裏面も表示)
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
		psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.RasterizerState.MultisampleEnable = FALSE;
		psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
		psoDesc.RasterizerState.ForcedSampleCount = 0;
		psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// デプスステンシル
		psoDesc.DepthStencilState.DepthEnable = TRUE; // まだ深度バッファがないのでOFF
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		// ====================================================
		// 1. 不透明用 PSO (Opaque)
		// ====================================================
		// ブレンド無効 (上書き)
		D3D12_RENDER_TARGET_BLEND_DESC opaqueBlend = {};
		opaqueBlend.BlendEnable = FALSE;
		opaqueBlend.LogicOpEnable = FALSE;
		opaqueBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[0] = opaqueBlend;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState))))
		{
			SPAN_ERROR("Failed to create Opaque PSO!");
			return false;
		}

		// ====================================================
		// 2. 透明用 PSO (Transparent)
		// ====================================================
		// アルファブレンド設定
		// FinalColor = (SrcColor * SrcAlpha) + (DestColor * (1 - SrcAlpha))
		D3D12_RENDER_TARGET_BLEND_DESC transBlend = {};
		transBlend.BlendEnable = TRUE;
		transBlend.LogicOpEnable = FALSE;
		transBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		transBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		transBlend.BlendOp = D3D12_BLEND_OP_ADD;
		transBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
		transBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
		transBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		transBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		psoDesc.BlendState.RenderTarget[0] = transBlend;

		// 半透明物体は深度バッファに書き込まないことが多い
		// (後ろのものが描画されなくなるのを防ぐため。ただし描画順序管理が必要)
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateTransparent))))
		{
			SPAN_ERROR("Failed to create Transparent PSO!");
			return false;
		}
		return true;
	}
}
