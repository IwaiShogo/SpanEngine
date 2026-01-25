#include "Core/CoreMinimal.h"
#include "Core/Time/Time.h"
#include "Core/Input/Input.h"
#include "Core/Math/SpanMath.h"
#include "Runtime/EntryPoint.h"
#include "Runtime/Application.h"

// ユーザーアプリケーションの定義
class PlayergroundApp : public Span::Application
{
public:
	PlayergroundApp()
	{
		// アプリケーション設定
	}

	~PlayergroundApp()
	{

	}

	void OnStart() override
	{
		// ゲームの初期化
	}

	void OnUpdate() override
	{
		// アプリケーション固有の更新処理
	}
};

Span::Application* Span::CreateApplication()
{
    return new PlayergroundApp();
}