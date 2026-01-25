#pragma once
#include "Core/CoreMinimal.h"
#include "Platform/Window.h"
#include "Graphics/Renderer.h"
#include "Graphics/Core/GraphicsContext.h"
#include "Graphics/Core/RenderTarget.h"
#include "ECS/Kernel/World.h"

namespace Span
{
	class Application
	{
	public:
		Application();
		virtual ~Application();

		// アプリを実行する
		void Run();

		// --- ユーザーがオーバーライドする関数 ---
		virtual void OnStart() {}
		virtual void OnUpdate() {}
		virtual void OnShutdown() {}

		// どこからでもアプリ本体、ひいてはWindowやWorldにアクセス可能にする
		static Application& Get() { return *s_instance; }

		Window& GetWindow() { return window; }
		Renderer& GetRenderer() { return renderer; }

		World& GetWorld() { return world; }
	protected:
		// 子クラス（ユーザーのゲーム）からアクセスできるようにする

	private:
		static Application* s_instance;
		bool isRunning = true;

		Window window;
		GraphicsContext graphicsContext;
		Renderer renderer;
		RenderTarget sceneBuffer;	// シーン描画用
		World world;
	};

	// ユーザー側で定義してもらう関数（ファクトリー関数）
	Application* CreateApplication();
}
