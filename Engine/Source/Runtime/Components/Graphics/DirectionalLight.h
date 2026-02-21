/*****************************************************************//**
 * @file	DirectionalLight.h
 * @brief	シーン全体に降り注ぐ平行光源 (太陽光) のコンポーネント。
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
#include "Runtime/Reflection/SpanReflection.h"

namespace Span
{
	/**
	 * @struct	DirectionalLight
	 * @brief	☀️ 距離による減衰の無い平行光源 (Directional Light) コンポーネント。
	 *
	 * @details
	 * Entity が持つ Transform コンポーネントの Rotation (回転) を「光の向き」として使用します。
	 * 基本的にシーン内に1つだけアクティブなものが存在することを想定します。
	 */
	struct DirectionalLight
	{
		Vector3 Color = { 1.0f, 1.0f, 1.0f };	///< 光の基本色 (RGB)
		float Intensity = 3.0f;					///< 光の強さ (輝度)

		bool CastShadows = true;				///< 影を生成するかどうか

		DirectionalLight() = default;

		SPAN_INSPECTOR_BEGIN(DirectionalLight)
			SPAN_FIELD(Color)
			SPAN_FIELD(Intensity, Min(0.0f))
			SPAN_FIELD(CastShadows)
		SPAN_INSPECTOR_END()
	};
}
