/*****************************************************************//**
 * @file	Logger.h
 * @brief	デバッグログ出力システム
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
	/// @brief	ログの重要度レベル
	enum class LogLevel : uint8
	{
		Info,		///< ℹ️ 一般的な情報 (白)
		Warning,	///< ⚠ 注意・警告 (黄)
		Error,		///< ❌ エラー (赤)
		Fatal		///< 💀 致命的エラー (赤背景・強制終了)
	};

	/**
	 * @class	Logger
	 * @brief	📝 コンソールおよびファイルへのログ出力を管理する静的クラス。
	 * 
	 * @details
	 * Visual Studioの出力ウィンドウ、標準コンソール、ログファイル(`Logs/SpanEngine.log`)の
	 * 3箇所に同時に出力します。
	 */
	class Logger
	{
	public:
		/// @brief	ロガーを初期化し、ログファイルを開きます。
		static void Initialize();

		/// @brief	ログファイルをフラッシュして閉じます。
		static void Shutdown();

		/**
		 * @brief	ログを出力します。
		 * @param	level 重要度
		 * @param	file 発生したファイル名 (__FILE__)
		 * @param	line 発生した行番号 (__LINE__)
		 * @param	format printf形式のフォーマット文字列
		 * @param	... 可変長引数
		 */
		static void Log(LogLevel level, const char* file, int line, const char* format, ...);

	private:
		static void SetConsoleColor(LogLevel level);
		static void ResetConsoleColor();
	};
}

// 📢 Log Macros
// ============================================================
// __FILE__ マクロなどを使って自動的に発生個所を記録します。

#undef SPAN_LOG
#undef SPAN_WARN
#undef SPAN_ERROR
#undef SPAN_FATAL

/// @brief	通常ログを出力します。
#define SPAN_LOG(Format, ...)	Span::Logger::Log(Span::LogLevel::Info,		__FILE__, __LINE__, Format, ##__VA_ARGS__)

/// @brief	警告ログを出力します (黄色)。
#define SPAN_WARN(Format, ...)	Span::Logger::Log(Span::LogLevel::Warning,	__FILE__, __LINE__, Format, ##__VA_ARGS__)

/// @brief	エラーログを出力します (赤色)。
#define SPAN_ERROR(Format, ...)	Span::Logger::Log(Span::LogLevel::Error,	__FILE__, __LINE__, Format, ##__VA_ARGS__)

/// @brief	致命的エラーを出力し、デバッガを停止させます。
#define SPAN_FATAL(Format, ...)	Span::Logger::Log(Span::LogLevel::Fatal,	__FILE__, __LINE__, Format, ##__VA_ARGS__)
