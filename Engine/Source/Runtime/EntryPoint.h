/*****************************************************************//**
 * @file	EntryPoint.h
 * @brief	アプリケーションのエントリーポイント (main関数)。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Application.h"

// ユーザー側で定義してもらう関数
extern Span::Application* Span::CreateApplication();

/**
 * @brief	メイン関数。
 *
 * @details
 * Windowsアプリケーションのエントリーポイントです。
 * ユーザーが定義した `CreateApplication` を呼び出し、ゲームループを開始します。
 * コンソールウィンドウを表示しない設定にする場合は、リンカー設定で `SubSystem:Windows` に変更してください。
 */
int main(int argc, char** argv)
{
	// メモリリーク検出 (Debugのみ)
#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	auto app = Span::CreateApplication();

	if (app)
	{
		app->Run();
		delete app;
	}

	return 0;
}
