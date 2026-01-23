#pragma once
#include "Application.h"

// ユーザーの代わりに main 関数を定義してしまう
extern Span::Application* Span::CreateApplication();

int main(int argc, char** argv)
{
	// ユーザーが作ったアプリのインスタンスをもらう
	auto app = Span::CreateApplication();

	// 実行
	app->Run();

	// 削除
	delete app;

	return 0;
}