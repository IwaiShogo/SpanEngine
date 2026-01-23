#pragma once
#include "Core/CoreMinimal.h"
#include "Platform/Window.h"
#include "ECS/Kernel/World.h"
#include "Graphics/Renderer.h"

namespace Span
{
	class Application
	{
	public:
		Application();
		virtual ~Application();

		// どこからでもアプリ本体、ひいてはWindowやWorldにアクセス可能にする
		static Application& Get() { return *s_instance; }

		Window& GetWindow() { return window; }
		Renderer& GetRenderer() { return renderer; }

		// アプリを実行する
		void Run();

		// --- ユーザーがオーバーライドする関数 ---
		virtual void OnStart() {}
		virtual void OnUpdate() {}
		virtual void OnShutdown() {}

	protected:
		// 子クラス（ユーザーのゲーム）からアクセスできるようにする
		World* GetWorld() { return &world; }

	private:
		static Application* s_instance;

		bool isRunning = true;
		Window window;	// Windowを待つ
		World world;	// ECS Worldを持つ

		Renderer renderer;
	};

	// ユーザー側で定義してもらう関数（ファクトリー関数）
	Application* CreateApplication();
}
