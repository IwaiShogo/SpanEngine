/*****************************************************************//**
 * @file	Window.h
 * @brief	Windows OSS ウィンドウのラッパー。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	/// @brief	ウィンドウ作成時の設定構造体
	struct WindowDesc
	{
		std::wstring Title = L"Span Engine";
		int Width = 1280;
		int Height = 720;
	};

	/**
	 * @class	Window
	 * @brief	🪟 OSネイティブウィンドウを管理するクラス。
	 * 
	 * @details
	 * Win32 API の `CreateWindowEx` やメッセージポンプ(`PeekMessage`) を隠蔽します。
	 * また、`GWLP_USERDATA` を使用して、静的な `WindowProc` からインスタンスのメンバ関数へ
	 * イベントを転送する仕組みを持っています。
	 */
	class Window
	{
	public:
		Window() = default;
		~Window();

		/**
		 * @brief	ウィンドウクラスの登録とウィンドウ作成を行います。
		 * @param	desc 設定データ
		 * @return	成功なら true
		 */
		bool Initialize(const WindowDesc& desc);

		/// @brief	ウィンドウを破棄します。
		void Shutdown();

		/**
		 * @brief	OS空のイベントメッセージを処理します (Message Pump)。
		 * @return	アプリケーションを継続すべきなら true、終了(WM_QUIT)なら false
		 */
		bool PollEvents();

		// 📊 Getters
		// ============================================================

		HWND GetHandle() const { return hWnd; }

		/// @brief	画面サイズ取得 (Width)
		int GetWidth() const { return width; }

		/// @brief	画面サイズ取得 (Height)
		int GetHeight() const { return height; }

		/// @brief	ウィンドウサイズ変更時に呼ばれるコールバックを登録します。
		void SetOnResize(std::function<void(uint32, uint32)> callback) { onResize = callback; }

	private:
		/**
		 * @brief	Win32 API用の静的コールバック関数
		 * 
		 * @details
		 * 最初の `WM_NCCREATE` メッセージで `this` ポインタを `GWLP_USERDATA` に保存し、
		 * 以降のメッセージではそれを取り出してインスタンスメンバにアクセスします。
		 */
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		HWND hWnd = nullptr;
		int width = 0;
		int height = 0;

		std::function<void(uint32, uint32)> onResize;
	};
}

