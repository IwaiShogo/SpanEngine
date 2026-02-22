#include "Application.h"
#include <SpanEngine.h>

#include "Editor/GuiManager.h"

#include "Editor/Panels/SceneView/SceneViewPanel.h"
#include "Runtime/Graphics/Passes/GridPass.h"

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
			if (w == 0 || h == 0) return;
			renderer.OnResize(w, h);
			});

		// Editor GUI (ImGui)
		GuiManager::Initialize(window.GetHandle(), renderer.GetDevice(), renderer.GetCommandQueue(), renderer.GetFrameCount());

		// Time
		Time::Initialize();

		// Input
		Input::Initialize(window.GetHandle());

		// AssetManager
		auto device = renderer.GetDevice();
		auto queue = renderer.GetCommandQueue();

		if (device && queue)
		{
			AssetManager::Get().Initialize(device, queue);
		}
		else
		{
			SPAN_ERROR("Failed to initialize AssetManager: Device or Queue is null");
		}

		SPAN_LOG("--- Initialization Complete ---");
	}

	Application::~Application()
	{
		// GPUの完了を確実に待つ
		graphicsContext.WaitForGpu();

		// ワールド内のシステム終了処理
		GetWorld().ShutdownSystem();

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
			if (!window.PollEvents())
			{
				isRunning = false;
				break;
			}

			if (window.GetWidth() == 0 || window.GetHeight() == 0)
			{
				Sleep(10);
				continue;
			}

			// 1. SceneBuffer Resize Check
			// ------------------------------------------------------------
			if (auto scenePanel = GuiManager::GetPanel<SceneViewPanel>())
			{
				Vector2 viewSize = scenePanel->GetTargetResolution();

				// サイズが有効かつ、現在と異なる場合のみリサイズ
				if (viewSize.x > 1.0f && viewSize.y > 1.0f)
				{
					uint32 targetW = (uint32)viewSize.x;
					uint32 targetH = (uint32)viewSize.y;

					// サイズが異なる場合のみ再構築
					if (sceneBuffer.GetWidth() != targetW || sceneBuffer.GetHeight() != targetH)
					{
						graphicsContext.WaitForGpu();
						sceneBuffer.Resize(renderer.GetDevice(), targetW, targetH);

						// 更新されたサイズを保持
						m_sceneViewWidth = targetW;
						m_sceneViewHeight = targetH;
					}
				}
			}

			// 2. Scene Rendering
			// ------------------------------------------------------------
			// コマンドリスト取得
			ID3D12GraphicsCommandList* cmd = renderer.BeginFrame();

			graphicsContext.SetRenderTargetToBackBuffer(cmd);
			GuiManager::BeginFrame();

			// ドッキングスペースの作成
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport());

			// --- A. シーン描画 (Render to Texture) ---
			{
				// バリア: SRV -> RTV
				sceneBuffer.TransitionToRenderTarget(cmd);

				// レンダーターゲット設定
				D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneBuffer.GetRTV();
				D3D12_CPU_DESCRIPTOR_HANDLE dsv = sceneBuffer.GetDSV();
				cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

				sceneBuffer.Clear(cmd);

				// クリア & ビューポート
				D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)sceneBuffer.GetWidth(), (float)sceneBuffer.GetHeight(), 0.0f, 1.0f };
				D3D12_RECT scissor = { 0, 0, (LONG)sceneBuffer.GetWidth(), (LONG)sceneBuffer.GetHeight() };
				cmd->RSSetViewports(1, &viewport);
				cmd->RSSetScissorRects(1, &scissor);

				// Logic Update & ECS Draw
				Time::Update();			// Time update
				Input::Update();		// Input update
				GetWorld().UpdateSystems();	// Systems update
				OnUpdate();				// User update

				GetWorld().ForEach<Camera, Transform>([&](Entity, Camera&, Transform& t)
				{
					renderer.SetCameraPosition(t.Position);
				});

				// バリア: RTV -> SRV
				sceneBuffer.TransitionToShaderResource(cmd);
			}

			// 3. Editor UI Rendering
			// ------------------------------------------------------------
			graphicsContext.SetRenderTargetToBackBuffer(cmd);

			// シーンビューへのテクスチャセット
			if (auto scenePanel = GuiManager::GetPanel<SceneViewPanel>())
			{
				D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = sceneBuffer.GetSRV();
				if (srvHandle.ptr != 0)
				{
					D3D12_GPU_DESCRIPTOR_HANDLE imGuiTexture = GuiManager::RegisterTexture(srvHandle);
					scenePanel->SetTexture(imGuiTexture);
				}
			}

			// 描画終了
			GuiManager::EndFrame(cmd);

			renderer.EndFrame();
			Input::EndFrame();
		}

		OnShutdown();
	}
}

