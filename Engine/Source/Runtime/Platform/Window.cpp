#include "Window.h"
#include "Core/Input/Input.h"

#include <imgui.h>

// ImGuiのWin32ハンドラ定義
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Span
{
	Window::~Window()
	{
		Shutdown();
	}

	bool Window::Initialize(const WindowDesc& desc)
	{
		width = desc.Width;
		height = desc.Height;

		// 1. ウィンドウクラスの登録
		WNDCLASSEX wc = { 0 };
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.lpszClassName = L"SpanEngineWindowClass";

		RegisterClassEx(&wc);

		// 2. ウィンドウサイズの調整
		RECT windowRect = { 0, 0, width, height };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		// 3. ウィンドウ作成
		hWnd = CreateWindowEx(
			0,
			L"SpanEngineWindowClass",
			desc.Title.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr, nullptr, GetModuleHandle(nullptr), nullptr
		);

		if (!hWnd)
		{
			SPAN_ERROR("Failed to create window!");
			return false;
		}

		// ユーザーデータをこのクラスのポインタに紐づける
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		// 4. 表示
		ShowWindow(hWnd, SW_SHOW);
		return true;
	}

	void Window::Shutdown()
	{
		if (hWnd)
		{
			DestroyWindow(hWnd);
			hWnd = nullptr;
		}
	}

	bool Window::PollEvents()
	{
		MSG msg = { 0 };
		// メッセージがある限り処理する
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return false; // アプリ終了シグナル
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true; // 継続
	}

	LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) return true;

		Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		switch (message)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		// リサイズイベント
		case WM_SIZE:
			if (window && window->onResize)
			{
				uint32 w = LOWORD(lParam);
				uint32 h = HIWORD(lParam);
				// 0サイズは無視
				if (w > 0 && h > 0)
				{
					window->width = w;
					window->height = h;
					window->onResize(w, h);
				}
			}
			return 0;

		// --- キーボード入力 ---
		case WM_KEYDOWN:
			Input::OnKeyDown((uint32)wParam);
			return 0;
		case WM_KEYUP:
			Input::OnKeyUp((uint32)wParam);
			return 0;

		// --- マウス入力 ---
		case WM_LBUTTONDOWN: Input::OnMouseDown(0); return 0;
		case WM_RBUTTONDOWN: Input::OnMouseDown(1); return 0;
		case WM_MBUTTONDOWN: Input::OnMouseDown(2); return 0;

		case WM_LBUTTONUP: Input::OnMouseUp(0); return 0;
		case WM_RBUTTONUP: Input::OnMouseUp(1); return 0;
		case WM_MBUTTONUP: Input::OnMouseUp(2); return 0;

		case WM_MOUSEMOVE:
			// クライアント領域内の座標を取得
			Input::OnMouseMove(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
