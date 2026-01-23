#include "Application.h"
#include "Core/Time/Time.h"

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
	}

	Application::~Application()
	{
		OnShutdown();
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

			// 時間更新
			Time::Update();

			// 描画開始
			renderer.BeginFrame();

			// システムの一括更新
			world.UpdateSystems();
			// ユーザー定義の更新処理
			OnUpdate();

			// 描画終了
			renderer.EndFrame();
		}

		world.ShutdownSystem();
	}
}
