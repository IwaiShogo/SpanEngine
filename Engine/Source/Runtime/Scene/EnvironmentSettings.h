/*****************************************************************//**
 * @file	EnvironmentSettings.h
 * @brief	シーンの環境光、スカイボックス、フォグなどの設定データ
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
	 * @struct	EnvironmentSettings
	 * @brief	🌏 シーン全体の環境 (空、光、雰囲気) を定義するデータ。
	 */
	struct EnvironmentSettings
	{
		// --- Skybox Settings ---
		bool UseProceduralSky = true;
		uint64_t SkyboxHDRI = 0;

		// デフォルトの3色グラデーション
		float SkyTopColor[3] = { 0.35f, 0.5f, 0.7f };		// 青色
		float SkyHorizonColor[3] = { 0.7f, 0.75f, 0.8f };	// 薄い水色/グレー
		float SkyBottomColor[3] = { 0.2f, 0.2f, 0.2f };		// 暗いグレー

		// --- Lighting & Ambient ---
		float AmbientIntensity = 1.0f;
		float EnvReflectionIntensity = 1.0f;
		float Exposure = 1.0f;	// トーンマッピング用
	};
}
