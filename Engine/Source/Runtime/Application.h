/*****************************************************************//**
 * @file	Application.h
 * @brief	エンジンのメインループとライフサイクル管理。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Platform/Window.h"
#include "Graphics/Renderer.h"
#include "Graphics/Core/GraphicsContext.h"
#include "Graphics/Core/RenderTarget.h"
#include "ECS/Kernel/World.h"
#include "Scene/Scene.h"

namespace Span
{
	/**
	 * @class	Application
	 * @brief	🚀 アプリケーションの基底クラス (シングルトン)。
	 *
	 * @details
	 * WinMainから呼び出され、ウィンドウ作成、レンダラー初期化、メインループ(`Run`)の実行を行います。
	 * ユーザーはこれを継承して `MyGame` クラスを作成します。
	 */
	class Application
	{
	public:
		Application();
		virtual ~Application();

		/// @brief	シングルトンインスタンスの取得
		static Application& Get() { return *s_instance; }

		/// @brief	シングルトンインスタンスのポインタ取得 (存在チェック用)
		static Application* Ptr() { return s_instance; }

		/// @brief	アプリケーションを終了モードにします
		void Close() { isRunning = false; }

		/**
		 * @brief	アプリケーションを実行します (ブロック関数)。
		 * @details	ウィンドウが閉じられるまで無限ループします。
		 */
		void Run();

		// 🎮 User Callbacks (Override these)
		// ============================================================

		/// @brief	起動時 (ループ開始前) に一度だけ呼ばれます。
		virtual void OnStart() {}

		/// @brief	毎フレーム呼ばれます。ゲームロジックをここに記述します。
		virtual void OnUpdate() {}

		/// @brief	終了時に呼ばれます。
		virtual void OnShutdown() {}

		// 🔧 System Accessors
		// ============================================================

		Window& GetWindow() { return window; }
		Renderer& GetRenderer() { return renderer; }
		World& GetWorld() { return activeScene.ECSWorld; }
		Scene& GetActiveScene() { return activeScene;  }

		/// @brief	エディタ表示用のシーン描画ターゲットを取得
		RenderTarget& GetSceneBuffer() { return sceneBuffer; }

		/// @brief	SceneViewPanelから毎フレームサイズを受けとる
		void SetSceneViewSize(uint32 width, uint32 height)
		{
			// 0除算防止
			if (width == 0 || height == 0) return;
			m_sceneViewWidth = width;
			m_sceneViewHeight = height;
		}

		float GetSceneViewAspectRatio() const
		{
			return (float)m_sceneViewWidth / (float)m_sceneViewHeight;
		}

	protected:
		// 子クラス（ユーザーのゲーム）からアクセスできるようにする

	private:
		static Application* s_instance;
		bool isRunning = true;

		// Systems
		Window window;
		GraphicsContext graphicsContext;
		Renderer renderer;
		Scene activeScene;

		// Editor Support
		RenderTarget sceneBuffer;	///< シーン描画用オフスクリーンバッファ

		// SceneView
		uint32 m_sceneViewWidth = 1280;
		uint32 m_sceneViewHeight = 720;
	};

	/// @brief	ユーザー側で定義すべきファクトリー関数
	Application* CreateApplication();
}

