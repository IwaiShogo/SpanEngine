#include "Application.h"
#include "Core/Time/Time.h"
#include "Core/Input/Input.h"
#include "Editor/GuiManager.h"

#include "Editor/Panels/SceneView/SceneViewPanel.h"

namespace Span
{
	Application* Application::s_instance = nullptr;

	Application::Application()
	{
		assert(!s_instance && "Application already exists!");	// 二重起動防止
		s_instance = this;

		// 1. ロガー初期化
		Logger::Initialize();
		SPAN_LOG("--- Span Engine Initializing ---");

		// 2. ウィンドウ初期化
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

		// 3. graphicsContext初期化
		if (!graphicsContext.Initialize(window))
		{
			SPAN_FATAL("GraphicsContext Initialization Failed!");
			isRunning = false;
			return;
		}

		// 4. レンダラー初期化
		if (!renderer.Initialize(&graphicsContext))
		{
			SPAN_FATAL("Render Initialization Failed!");
			isRunning = false;
			return;
		}

		// 4-b. シーンバッファの初期化
		if (!sceneBuffer.Initialize(renderer.GetDevice(), window.GetWidth(), window.GetHeight()))
		{
			SPAN_FATAL("SceneBuffer Initialization Failed!");
			isRunning = false;
		}

		// ウィンドウリサイズ時にレンダラーへ通知
		window.SetOnResize([this](uint32 w, uint32 h) {
			renderer.OnResize(w, h);
			sceneBuffer.Resize(renderer.GetDevice(), w, h);
			});

		// 5. 時間・入力・GUI管理初期化
		Time::Initialize();
		Input::Initialize(window.GetHandle());
		GuiManager::Initialize(window.GetHandle(), renderer.GetDevice(), renderer.GetCommandQueue(), renderer.GetFrameCount());
	}

	Application::~Application()
	{
		OnShutdown();
		GuiManager::Shutdown();
		sceneBuffer.Shutdown();
		renderer.Shutdown();
		graphicsContext.Shutdown();
		window.Shutdown();
		Logger::Shutdown();
	}

	void Application::Run()
	{
		// ユーザー定義の開始処理
		OnStart();

		while (isRunning)
		{
			// ウィンドウイベント処理
			if (!window.PollEvents())
			{
				isRunning = false;
				break;
			}

			// --- 1. シーン描画 (Render to Texture) ---

			// 戻り値を受け取るように変更 (E0144修正)
			ID3D12GraphicsCommandList* cmd = renderer.BeginFrame();

			sceneBuffer.TransitionToRenderTarget(cmd);

			D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneBuffer.GetRTV();

			// RenderTarget.h が正しく更新されていれば GetDSV() が使えるはず (E0135修正)
			// もしエラーが続く場合は、RenderTarget.hにGetDSV()があるか確認してください
			D3D12_CPU_DESCRIPTOR_HANDLE dsv = sceneBuffer.GetDSV();

			cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
			sceneBuffer.Clear(cmd);

			// ビューポート設定
			D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)sceneBuffer.GetWidth(), (float)sceneBuffer.GetHeight(), 0.0f, 1.0f };
			D3D12_RECT scissor = { 0, 0, (LONG)sceneBuffer.GetWidth(), (LONG)sceneBuffer.GetHeight() };
			cmd->RSSetViewports(1, &viewport);
			cmd->RSSetScissorRects(1, &scissor);

			Time::Update();
			Input::Update();
			world.UpdateSystems();
			OnUpdate();

			sceneBuffer.TransitionToShaderResource(cmd);
			

			// --- 2. エディタ描画 (Back Buffer) ---

			// SceneViewPanelに最新のテクスチャを渡す
			if (auto scenePanel = GuiManager::GetPanel<SceneViewPanel>())
			{
				D3D12_GPU_DESCRIPTOR_HANDLE imGuiTexture = GuiManager::RegisterTexture(sceneBuffer.GetSRV());
				scenePanel->SetTexture(imGuiTexture);
			}

			// ImGui用にバックバッファへ戻す (GraphicsContextに追加した関数を使う)
			graphicsContext.SetRenderTargetToBackBuffer(cmd);

			GuiManager::BeginFrame();
			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

			GuiManager::EndFrame(cmd);

			renderer.EndFrame();

			Input::EndFrame();
		}

		world.ShutdownSystem();
	}
}
