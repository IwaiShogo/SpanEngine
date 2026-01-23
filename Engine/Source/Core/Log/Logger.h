#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	// ログレベル定義
	enum class LogLevel : uint8
	{
		Info,
		Warning,
		Error,
		Fatal
	};

	class Logger
	{
	public:
		// エンジン起動時にログファイルを開く
		static void Initialize();
		// 終了時にファイルを閉じる
		static void Shutdown();

		// ログ出力関数（ユーザーは直接呼ばず、マクロ経由で使う）
		static void Log(LogLevel level, const char* file, int line, const char* format, ...);

	private:
		static void SetConsoleColor(LogLevel level);
		static void ResetConsoleColor();
	};
}

// --------------------------------------------------------------------------------
// ログマクロ（使いやすくするためのラッパー）
// __FILE__, __LINE__ を自動で渡すことで、どのファイルの何行目で起きたか記録します。
// --------------------------------------------------------------------------------
#define SPAN_LOG(Format, ...)	Span::Logger::Log(Span::LogLevel::Info,		__FILE__, __LINE__, Format, ##__VA_ARGS__)
#define SPAN_WARN(Format, ...)	Span::Logger::Log(Span::LogLevel::Warning,	__FILE__, __LINE__, Format, ##__VA_ARGS__)
#define SPAN_ERROR(Format, ...)	Span::Logger::Log(Span::LogLevel::Error,	__FILE__, __LINE__, Format, ##__VA_ARGS__)
#define SPAN_FATAL(Format, ...)	Span::Logger::Log(Span::LogLevel::Fatal,	__FILE__, __LINE__, Format, ##__VA_ARGS__)