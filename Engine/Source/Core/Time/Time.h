/*****************************************************************//**
 * @file	Time.h
 * @brief	ゲームループの時間管理クラス。
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
	/**
	 * @class	Time
	 * @brief	⏱ 時間経過 (Delta Time) と総時間を管理する静的クラス。
	 * 
	 * @details
	 * `std::chrono::high_resolution_clock` を使用してナノ秒制度の計測を行います。
	 * `Update()` はメインループの先頭で必ず呼び出す必要があります。
	 */
	class Time
	{
	public:
		/// @brief	初期化（エンジン起動時）
		static void Initialize();
		/// @brief	フレーム開始時に呼ぶ（時間を進める）
		static void Update();

		// --- パブリック API (ユーザーが使う) ---

		/// @brief	前のフレームからの経過時間 (秒) を取得します。
		/// @return	例: 60FPSなら約 0.01666...
		static float GetDeltaTime() { return deltaTime; }

		/// @brief	ゲーム開始時からの総経過時間 (秒) を取得します。
		static float GetTotalTime() { return totalTime; }

	private:
		static std::chrono::high_resolution_clock::time_point startTime;
		static std::chrono::high_resolution_clock::time_point lastFrameTime;
		static float deltaTime;
		static float totalTime;
	};
}

