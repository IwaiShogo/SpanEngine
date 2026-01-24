#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	struct WindowDesc
	{
		std::wstring Title = L"Span Engine";
		int Width = 1280;
		int Height = 720;
	};

	class Window
	{
	public:
		Window() = default;
		~Window();

		bool Initialize(const WindowDesc& desc);
		void Shutdown();

		// メッセージポンプ (ウィンドウのxボタンが押されたかチェック)
		// 戻り値: false ならアプリ終了
		bool PollEvents();

		// 内部ハンドル取得
		HWND GetHandle() const { return hWnd; }

		// 画面サイズ取得
		int GetWidth() const { return width; }
		int GetHeight() const { return height; }

		// リサイズ時のコールバック登録
		void SetOnResize(std::function<void(uint32, uint32)> callback) { onResize = callback; }

	private:
		HWND hWnd = nullptr;
		int width = 0;
		int height = 0;

		std::function<void(uint32, uint32)> onResize;

		// Windowsからのメッセージを受け取るコールバック
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
}
