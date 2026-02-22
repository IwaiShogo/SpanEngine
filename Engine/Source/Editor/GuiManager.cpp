#include "GuiManager.h"

#include <SpanEngine.h>

#include "PanelManager.h"
#include "Runtime/Scene/SceneSerializer.h"
#include "Editor/Utils/FileDialog.h"

#include <ImGuizmo.h>

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
		desc.NumDescriptors = 256;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap));
		if (FAILED(hr))
		{
			SPAN_ERROR("[GuiManager] Failed to create Descriptor Heap for ImGui! HRESULT: 0x%08X", hr);
			return;
		}

		// 4. プラットフォーム初期化
		ImGui_ImplWin32_Init(hWnd);

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

		ImGuizmo::BeginFrame();

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

		bool requestNewScene = false;
		bool requestOpenScene = false;
		bool requestSaveScene = false;
		bool requestSaveSceneAs = false;

		bool ctrl = ImGui::GetIO().KeyCtrl;
		bool shift = ImGui::GetIO().KeyShift;

		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N)) requestNewScene = true;
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O)) requestOpenScene = true;
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
			if (shift) requestSaveSceneAs = true;
			else	   requestSaveScene = true;
		}

		static bool s_ShowEnvironmentSettings = false;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene", "Ctrl+N")) requestNewScene = true;
				if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) requestOpenScene = true;
				ImGui::Separator();
				if (ImGui::MenuItem("Save", "Ctrl+S")) requestSaveScene = true;
				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) requestSaveSceneAs = true;
				ImGui::Separator();
				if (ImGui::MenuItem("Exit", "Alt+F4")) Application::Get().Close();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
				ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				ImGui::MenuItem("Environment Settings", nullptr, &s_ShowEnvironmentSettings);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// === ファイル操作の実行部 ===
		static std::string currentScenePath = "";

		if (requestNewScene)
		{
			Application::Get().GetActiveScene().ECSWorld.Clear();
			Application::Get().GetActiveScene().Name = "Untitled";
			currentScenePath = "";
			SPAN_LOG("Created new empty scene.");
		}

		if (requestOpenScene)
		{
			// std::optional で受け取る
			auto filepathOpt = FileDialog::OpenFile("Span Scene (*.span)\0*.span\0");
			if (filepathOpt.has_value())
			{
				std::string filepath = filepathOpt.value();
				SceneSerializer serializer(Application::Get().GetActiveScene());
				if (serializer.Deserialize(filepath))
				{
					currentScenePath = filepath;
					SPAN_LOG("Opened scene: %s", filepath.c_str());
				}
			}
		}

		if (requestSaveScene || requestSaveSceneAs)
		{
			std::string filepath = currentScenePath;

			if (requestSaveSceneAs || filepath.empty())
			{
				auto filepathOpt = FileDialog::SaveFile("Span Scene (*.span)\0*.span\0");
				if (filepathOpt.has_value()) filepath = filepathOpt.value();
				else filepath = ""; // キャンセルされた場合
			}

			if (!filepath.empty())
			{
				if (filepath.find(".span") == std::string::npos) filepath += ".span";
				SceneSerializer serializer(Application::Get().GetActiveScene());
				Application::Get().GetActiveScene().Name = std::filesystem::path(filepath).stem().string();

				if (serializer.Serialize(filepath))
				{
					currentScenePath = filepath;
					SPAN_LOG("Saved scene to: %s", filepath.c_str());
				}
			}
		}

		if (s_ShowEnvironmentSettings)
		{
			ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Environment Sttings", &s_ShowEnvironmentSettings))
			{
				auto& env = Application::Get().GetActiveScene().Environment;

				ImGui::SeparatorText("Skybox");

				ImGui::Checkbox("Use Procedural Sky", &env.UseProceduralSky);

				ImGui::SameLine();
				if (ImGui::Button("Reset Colors"))
				{
					env.SkyTopColor[0] = 0.35f; env.SkyTopColor[1] = 0.5f; env.SkyTopColor[2] = 0.7f;
					env.SkyHorizonColor[0] = 0.7f; env.SkyHorizonColor[1] = 0.75f; env.SkyHorizonColor[2] = 0.8f;
					env.SkyBottomColor[0] = 0.2f; env.SkyBottomColor[1] = 0.2f; env.SkyBottomColor[2] = 0.2f;
				}

				ImGuiColorEditFlags hdrFlags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;

				if (env.UseProceduralSky)
				{
					ImGui::Indent();
					ImGui::ColorEdit3("Top Color", env.SkyTopColor, hdrFlags);
					ImGui::ColorEdit3("Horizon Color", env.SkyHorizonColor, hdrFlags);
					ImGui::ColorEdit3("Bottom Color", env.SkyBottomColor, hdrFlags);
					ImGui::Unindent();
				}
				else
				{
					ImGui::Indent();
					ImGui::Text("HDRI Texture");
					// 将来的にここに HDRI テクスチャのドロップスロットを実装
					ImGui::Unindent();
				}

				ImGui::Spacing();
				ImGui::SeparatorText("Lighting & Camera Exposure");

				ImGui::SliderFloat("Ambient Intensity", &env.AmbientIntensity, 0.0f, 5.0f);
				ImGui::SliderFloat("Reflection Intensity", &env.EnvReflectionIntensity, 0.0f, 10.0f);
				ImGui::SliderFloat("Exposure", &env.Exposure, 0.1f, 10.0f);

				// 将来拡張用
				ImGui::Spacing();
				ImGui::SeparatorText("Fog (Coming Soon)");
				ImGui::BeginDisabled();
				bool fogEnable = false;
				float fogColor[3] = { 0.5f, 0.5f, 0.5f };
				ImGui::Checkbox("Enable Fog", &fogEnable);
				ImGui::ColorEdit3("Fog Color", fogColor);
				ImGui::EndDisabled();
			}
			ImGui::End();
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
		if (!m_device || !srvHeap || srcHandle.ptr == 0) return { 0 };

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
		style.WindowRounding    = 4.0f;
		style.ChildRounding     = 4.0f;
		style.FrameRounding     = 4.0f;
		style.GrabRounding      = 4.0f;
		style.PopupRounding     = 4.0f;
		style.ScrollbarRounding = 4.0f;
		style.TabRounding       = 4.0f;

		// 色のカスタマイズ (Dark Theme風)
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

