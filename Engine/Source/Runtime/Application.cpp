#include "Application.h"
#include "Core/Time/Time.h"
#include "Core/Input/Input.h"
#include "Editor/GuiManager.h"

#include "Editor/Panels/SceneView/SceneViewPanel.h"

namespace Span
{
	Application* Application::s_instance = nullptr;

	// コンソールイベントハンドラ
	static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
	{
		// コンソールのxボタンが押された場合
		if (dwCtrlType == CTRL_CLOSE_EVENT)
		{
			if (auto* app = Application::Ptr())
			{
				// 1. メインループを止めるフラグを立てる
				app->Close();

				// 2. メインスレッドが完全に終了し、インスタンスが破棄されるのを待つ
				int safetyCount = 200;
				while (Application::Ptr() && safetyCount > 0)
				{
					Sleep(10);	// 10ms
					safetyCount--;
				}
			}
			return TRUE;
		}
		return FALSE;
	}

	Application::Application()
		: m_sceneViewWidth(1280)
		, m_sceneViewHeight(720)
	{
		assert(!s_instance && "Application already exists!");	// 二重起動防止
		s_instance = this;

		// コンソール終了ハンドラの登録
		SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

		// 1. システム初期化 (System Initialization)
		// ------------------------------------------------------------

		Logger::Initialize();
		SPAN_LOG("--- Span Engine Initializing ---");

		// Window
		WindowDesc desc;
		desc.Title = L"Span Engine App";
		desc.Width = 1280;
		desc.Height = 720;

		if (!window.Initialize(desc))
		{
			SPAN_FATAL("Window Initialization Failed!");
			isRunning = false;
			return;
		}

		// Graphics Context (DX12)
		if (!graphicsContext.Initialize(window))
		{
			SPAN_FATAL("GraphicsContext Initialization Failed!");
			isRunning = false;
			return;
		}

		// Renderer
		if (!renderer.Initialize(&graphicsContext))
		{
			SPAN_FATAL("Render Initialization Failed!");
			isRunning = false;
			return;
		}

		// SceneBuffer (Render Target for Editor View)
		if (!sceneBuffer.Initialize(renderer.GetDevice(), window.GetWidth(), window.GetHeight()))
		{
			SPAN_FATAL("SceneBuffer Initialization Failed!");
			isRunning = false;
		}

		// Resize
		window.SetOnResize([this](uint32 w, uint32 h) {
			renderer.OnResize(w, h);
			sceneBuffer.Resize(renderer.GetDevice(), w, h);
			});

		// Editor GUI (ImGui)
		GuiManager::Initialize(window.GetHandle(), renderer.GetDevice(), renderer.GetCommandQueue(), renderer.GetFrameCount());

		// Time
		Time::Initialize();

		// Input
		Input::Initialize(window.GetHandle());

		SPAN_LOG("--- Initialization Complete ---");
	}

	Application::~Application()
	{
		// GPUの完了を確実に待つ
		graphicsContext.WaitForGpu();

		// ワールド内のシステム終了処理
		world.ShutdownSystem();

		GuiManager::Shutdown();
		sceneBuffer.Shutdown();
		renderer.Shutdown();

		// 全てのresourceが消えた後に Contextを消す
		graphicsContext.Shutdown();
		window.Shutdown();
		Logger::Shutdown();

		s_instance = nullptr;
	}

	void Application::Run()
	{
		if (!isRunning) return;

		// ユーザー定義の開始処理
		OnStart();

		// Main Loop
		// ------------------------------------------------------------
		while (isRunning)
		{
			// 1. OSイベント処理 (閉じるボタンなど)
			if (!window.PollEvents())
			{
				isRunning = false;
				break;
			}

			// 2. リサイズ対応 (シーンバッファ)
			// シーンビューパネルのサイズに合わせてRTをリサイズする
			if (auto scenePanel = GuiManager::GetPanel<SceneViewPanel>())
			{
				Vector2 viewSize = scenePanel->GetTargetResolution();
				if (viewSize.x > 0 && viewSize.y > 0)
				{
					if (sceneBuffer.GetWidth() != (uint32)viewSize.x || sceneBuffer.GetHeight() != (uint32)viewSize.y)
					{
						graphicsContext.WaitForGpu();
						renderer.OnResize((uint32)viewSize.x, (uint32)viewSize.y);
						sceneBuffer.Resize(renderer.GetDevice(), (uint32)viewSize.x, (uint32)viewSize.y);
					}
				}
			}

			// 🖌 Rendering Phase
			// ============================================================

			// コマンドリスト取得
			ID3D12GraphicsCommandList* cmd = renderer.BeginFrame();

			// --- A. シーン描画 (Render to Texture) ---
			{
				// 必要であればレンダーターゲットをリサイズ
				if (m_sceneViewWidth > 0 && m_sceneViewHeight > 0)
				{
					if (sceneBuffer.GetWidth() != m_sceneViewWidth || sceneBuffer.GetHeight() != m_sceneViewHeight)
					{
						renderer.WaitForGPU();

						sceneBuffer.Resize(renderer.GetDevice(), m_sceneViewWidth, m_sceneViewHeight);
					}
				}

				// バリア: SRV -> RTV
				sceneBuffer.TransitionToRenderTarget(cmd);

				// レンダーターゲット設定
				D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneBuffer.GetRTV();
				D3D12_CPU_DESCRIPTOR_HANDLE dsv = sceneBuffer.GetDSV();
				cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

				// クリア & ビューポート
				sceneBuffer.Clear(cmd);
				D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)sceneBuffer.GetWidth(), (float)sceneBuffer.GetHeight(), 0.0f, 1.0f };
				D3D12_RECT scissor = { 0, 0, (LONG)sceneBuffer.GetWidth(), (LONG)sceneBuffer.GetHeight() };
				cmd->RSSetViewports(1, &viewport);
				cmd->RSSetScissorRects(1, &scissor);

				// Logic Update & ECS Draw
				Time::Update();			// Time update
				Input::Update();		// Input update
				world.UpdateSystems();	// Systems update
				OnUpdate();				// User update

				// バリア: RTV -> SRV
				sceneBuffer.TransitionToShaderResource(cmd);
			}

			// --- B. エディタ描画 (Back Buffer) ---

			// バックバッファへ戻す
			graphicsContext.SetRenderTargetToBackBuffer(cmd);

			// フレーム開始
			GuiManager::BeginFrame();

			// ドッキングスペースの作成
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport());

			// シーンビューへのテクスチャセット
			if (auto scenePanel = GuiManager::GetPanel<SceneViewPanel>())
			{
				D3D12_GPU_DESCRIPTOR_HANDLE imGuiTexture = GuiManager::RegisterTexture(sceneBuffer.GetSRV());
				scenePanel->SetTexture(imGuiTexture);
			}

			// 描画終了
			GuiManager::EndFrame(cmd);

			renderer.EndFrame();
			Input::EndFrame();
		}

		OnShutdown();
	}
}

