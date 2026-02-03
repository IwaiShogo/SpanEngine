#include "Logger.h"

namespace Span
{
	// ログファイルのポインタ
	static FILE* s_LogFile = nullptr;

	void Logger::Initialize()
	{
		// ログフォルダの作成 (Bin/Logs)
		std::filesystem::create_directories("Logs");

		// ファイルオープン
		errno_t err = fopen_s(&s_LogFile, "Logs/SpanEngine.log", "w");
		if (err != 0)
		{
			printf("Failed to open log file!\n");
		}
	}

	void Logger::Shutdown()
	{
		if (s_LogFile)
		{
			fclose(s_LogFile);
			s_LogFile = nullptr;
		}
	}

	void Logger::Log(LogLevel level, const char* file, int line, const char* format, ...)
	{
		// 1. メッセージのフォーマット
		const int bufferSize = 1024;
		char messageBuffer[bufferSize];

		va_list args;
		va_start(args, format);
		vsnprintf(messageBuffer, bufferSize, format, args);
		va_end(args);

		// 2. プレフィックスの作成（時刻やレベル、ファイル名）
		const char* levelStr = "";
		switch (level)
		{
		case LogLevel::Info:	levelStr = "[INFO] "; break;
		case LogLevel::Warning:	levelStr = "[WARN] "; break;
		case LogLevel::Error:	levelStr = "[ERR]  "; break;
		case LogLevel::Fatal:	levelStr = "[FATAL]"; break;
		}

		// ファイル名のみ抽出
		std::string fileName = std::filesystem::path(file).filename().string();

		char finalBuffer[2048];
		snprintf(finalBuffer, sizeof(finalBuffer), "%s%s (%s:%d)\n", levelStr, messageBuffer, fileName.c_str(), line);

		// 3. コンソール出力
		SetConsoleColor(level);
		printf("%s", finalBuffer);
		ResetConsoleColor();

		// 4. Visual Studioの出力ウィンドウへの出力
		OutputDebugStringA(finalBuffer);

		// 5. ファイル出力
		if (s_LogFile)
		{
			fprintf(s_LogFile, "%s", finalBuffer);
			fflush(s_LogFile);
		}

		// Fatalなら強制終了
		if (level == LogLevel::Fatal)
		{
			MessageBoxA(nullptr, finalBuffer, "Span Engine Fatal Error", MB_OK | MB_ICONERROR);
			__debugbreak();	// デバッガで止める
		}
	}

	void Logger::SetConsoleColor(LogLevel level)
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		switch (level)
		{
		case LogLevel::Info:	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); break;		// 白
		case LogLevel::Warning:	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); break;	// 黄色
		case LogLevel::Error:	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); break;					// 赤
		case LogLevel::Fatal:	SetConsoleTextAttribute(hConsole, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); break;	// 赤背景
		}
	}

	void Logger::ResetConsoleColor()
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
}

