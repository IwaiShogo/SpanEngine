#include "GuiManager.h"

#include <SpanEngine.h>

#include "PanelManager.h"

namespace Span
{
	ComPtr<ID3D12DescriptorHeap> GuiManager::srvHeap;
	std::vector<std::shared_ptr<EditorPanel>> GuiManager::panels;

	ID3D12Device* GuiManager::m_device = nullptr;

	void GuiManager::Initialize(HWND hWnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue,int numFrames)
	{
		if (!device)
		{
			SPAN_ERROR("[GuiManager] Device is NULL!");
			return;
		}

		m_device = device;

		// 1. ImGuiコンテキスト作成
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// ドッキングとビューポートを有効化
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		// 2. スタイル適用
		ApplyStyle();

		// 3. SRVヒープ作成
		srvHeap.Reset();

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 64;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap));
		if (FAILED(hr))
		{
			SPAN_ERROR("[GuiManager] Failed to create Descriptor Heap for ImGui! HRESULT: 0x%08X", hr);
			return;
		}

		// 4. プラットフォーム初期化
		if (!ImGui_ImplWin32_Init(hWnd))
		{
			SPAN_ERROR("[GuiManager] ImGui_ImplWin32_Init failed!");
			return;
		}

		// InitInfo構造体を使って初期化し、コマンドキューを渡す
		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = device;
		init_info.CommandQueue = commandQueue;
		init_info.NumFramesInFlight = numFrames;
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		init_info.DSVFormat = DXGI_FORMAT_D32_FLOAT; // Rendererの設定に合わせる

		// ユーザー定義のヒープを使う場合の設定
		init_info.SrvDescriptorHeap = srvHeap.Get();
		init_info.LegacySingleSrvCpuDescriptor = srvHeap->GetCPUDescriptorHandleForHeapStart();
		init_info.LegacySingleSrvGpuDescriptor = srvHeap->GetGPUDescriptorHandleForHeapStart();

		if (!ImGui_ImplDX12_Init(&init_info))
		{
			SPAN_ERROR("[GuiManager] ImGui_ImplDX12_Init failed!");
			return;
		}

		// フォント作成を明示的に呼び出し（任意だが、初期化確認のために推奨）
		if (!ImGui_ImplDX12_CreateDeviceObjects())
		{
			SPAN_ERROR("[GuiManager] CreateDeviceObjects failed! Check Debug Layer output.");
		}

		// 全パネルを一括生成
		auto autoPanels = PanelManager::CreateAllPanels();
		for (auto& p : autoPanels)
		{
			AddPanel(p);
		}

		SPAN_LOG("[GuiManager] Initialized Successfully.");
	}

	void GuiManager::Shutdown()
	{
		// Panels
		panels.clear();

		// ヒープも明示的にリセット
		srvHeap.Reset();

		if (m_device)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			m_device = nullptr;
		}
	}

	void GuiManager::BeginFrame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGuiIO& io = ImGui::GetIO();
		Input::SetImGuiWantCapture(io.WantCaptureMouse);
	}

	void GuiManager::EndFrame(ID3D12GraphicsCommandList* commandList)
	{
		// 登録された全パネルを描画
		for (auto& panel : panels)
		{
			if (panel->IsOpen())
			{
				panel->OnImGuiRender();
			}
		}

		ImGui::Render();

		// ヒープをセットして描画
		ID3D12DescriptorHeap* heaps[] = { srvHeap.Get() };
		commandList->SetDescriptorHeaps(1, heaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

		// マルチビューポート
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(nullptr, (void*)commandList);
		}
	}

	void GuiManager::AddPanel(std::shared_ptr<EditorPanel> panel)
	{
		panels.push_back(panel);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GuiManager::RegisterTexture(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
	{
		if (!m_device || !srvHeap) return { 0 };

		// SRVヒープの先頭ハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE destHandleCPU = srvHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE destHandleGPU = srvHeap->GetGPUDescriptorHandleForHeapStart();

		// インクリメントサイズを取得
		UINT handleIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// インデックス1番目を使用する (0番目はImGuiのフォント用に使われているため)
		int index = 1;

		destHandleCPU.ptr += (UINT64)index * handleIncrementSize;
		destHandleGPU.ptr += (UINT64)index * handleIncrementSize;

		// デバイスを使ってコピーを実行
		m_device->CopyDescriptorsSimple(1, destHandleCPU, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		return destHandleGPU;
	}

	void GuiManager::ApplyStyle()
	{
		// ベースはダークテーマ
		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();

		// 丸みを持たせる
		style.WindowRounding = 4.0f;
		style.ChildRounding = 4.0f;
		style.FrameRounding = 4.0f;
		style.GrabRounding = 4.0f;
		style.PopupRounding = 4.0f;
		style.ScrollbarRounding = 4.0f;
		style.TabRounding = 4.0f;

		// 色のカスタマイズ (Unity 2021+ Dark Theme風)
		ImVec4* colors = style.Colors;

		// 背景色: 少しマットなダークグレー
		colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

		// ヘッダー (タイトルバーなど)
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

		// ボタン
		colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

		// フレーム (入力欄など)
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

		// タブ
		colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

		// タイトルバー
		colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	}
}

