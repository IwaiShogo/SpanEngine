#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	class Time
	{
	public:
		// 初期化（エンジン起動時）
		static void Initialize();
		// フレーム開始時に呼ぶ（時間を進める）
		static void Update();

		// --- パブリック API (ユーザーが使う) ---

		// 前フレームからの経過時間 (秒)
		static float GetDeltaTime() { return deltaTime; }

		// ゲーム開始からの総時間 (秒)
		static float GetTotalTime() { return totalTime; }

	private:
		static std::chrono::high_resolution_clock::time_point startTime;
		static std::chrono::high_resolution_clock::time_point lastFrameTime;
		static float deltaTime;
		static float totalTime;
	};
}

