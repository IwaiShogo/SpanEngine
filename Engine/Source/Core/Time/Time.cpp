#include "Time.h"

namespace Span
{
	std::chrono::high_resolution_clock::time_point Time::startTime;
	std::chrono::high_resolution_clock::time_point Time::lastFrameTime;
	float Time::deltaTime = 0.0f;
	float Time::totalTime = 0.0f;

	void Time::Initialize()
	{
		startTime = std::chrono::high_resolution_clock::now();
		lastFrameTime = startTime;
		deltaTime = 0.0f;
		totalTime = 0.0f;
	}

	void Time::Update()
	{
		auto current = std::chrono::high_resolution_clock::now();

		// 経過時間を計算 (秒単位)
		std::chrono::duration<float> diff = current - lastFrameTime;
		deltaTime = diff.count();

		// 総時間を更新
		std::chrono::duration<float> total = current - startTime;
		totalTime = total.count();

		lastFrameTime = current;
	}
}

