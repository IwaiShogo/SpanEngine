#include "Application.h"
#include "Core/Time/Time.h"
#include "Core/Input/Input.h"
#include "Editor/GuiManager.h"

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

		// 3. レンダラー初期化
		if (!renderer.Initialize(window))
		{
			SPAN_FATAL("Renderer Initialization Failed!");
			isRunning = false;
			return;
		}

		// 4. 時間管理初期化
		Time::Initialize();
		Input::Initialize();
		GuiManager::Initialize(window.GetHandle(), renderer.GetDevice(), renderer.GetFrameCount());
	}

	Application::~Application()
	{
		OnShutdown();
		GuiManager::Shutdown();
		renderer.Shutdown();
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

			// 1. レンダリング開始
			renderer.BeginFrame();

			// 2. GUIの準備
			GuiManager::BeginFrame();

			// UI定義を書く
			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

			// テストウィンドウ
			ImGui::Begin("Inspector");
			ImGui::Text("Application Running...");
			ImGui::Text("Frame Rate: %.1f FPS", ImGui::GetIO().Framerate);
			ImGui::End();

			// 3. ゲームロジック更新
			Time::Update();
			Input::Update();

			// システムの一括更新
			world.UpdateSystems();
			// ユーザー定義の更新処理
			OnUpdate();

			// 4. GUIの描画
			GuiManager::EndFrame(renderer.GetCommandList());

			// 5. フレーム終了
			renderer.EndFrame();
		}

		world.ShutdownSystem();
	}
}
