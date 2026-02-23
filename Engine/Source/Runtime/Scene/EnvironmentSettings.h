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
#include "Core/Math/SpanMath.h"

namespace Span
{
	/**
	 * @enum	SkyboxMode
	 * @brief	スカイボックスのモード
	 */
	enum class SkyboxMode
	{
		Procedural = 0,	///< 3色グラデーション
		HDRI = 1		///< パノラマHDR画像
	};

	/**
	 * @struct	EnvironmentSettings
	 * @brief	🌏 シーン全体の環境 (空、光、雰囲気) を定義するデータ。
	 */
	struct EnvironmentSettings
	{
		// --- Skybox Settings ---
		SkyboxMode Mode = SkyboxMode::Procedural;
		std::string HDRIPath = "";	// 空の時は何もロードしない

		// デフォルトの3色グラデーション
		Vector3 SkyTopColor = { 0.35f, 0.5f, 0.7f };		// 青色
		Vector3 SkyHorizonColor = { 0.7f, 0.75f, 0.8f };	// 薄い水色/グレー
		Vector3 SkyBottomColor = { 0.2f, 0.2f, 0.2f };		// 暗いグレー

		// --- Lighting & Ambient ---
		float Exposure = 1.0f;					///< カメラの露出 (画面全体の明るさ)
		float AmbientIntensity = 1.0f;			///< 環境光の強さ
		float EnvReflectionIntensity = 1.0f;	///< 反射の強さ
	};
}
