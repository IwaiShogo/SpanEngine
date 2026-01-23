#include "Window.h"

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
		switch (message)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
