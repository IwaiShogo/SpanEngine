/*****************************************************************//**
 * @file	SpotLight.h
 * @brief	円錐状に照らすスポットライト
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
	 * @struct	SpotLight
	 * @brief	特定の方向を円錐状に照らす光源。
	 */
	struct SpotLight
	{
		Vector3 Color = { 1.0f, 1.0f, 1.0f };	///< 光の色
		float Intensity = 15.0f;				///< 光の強さ
		float Range = 15.0f;					///< 光が届く最大距離

		float InnerConeAngle = 15.0f;			///< 完全に明るい内側の角度 (度)
		float OuterConeAngle = 30.0f;			///< 光が減衰して消える外側の角度 (度)

		bool CastShadows = false;

		SpotLight() = default;

		SPAN_INSPECTOR_BEGIN(SpotLight)
			SPAN_FIELD(Color)
			SPAN_FIELD(Intensity, Min(0.0f))
			SPAN_FIELD(Range, Min(0.1f))
			SPAN_FIELD(InnerConeAngle, Span::Range(1.0f, 89.0f))
			SPAN_FIELD(OuterConeAngle, Span::Range(1.0f, 90.0f))
			SPAN_FIELD(CastShadows)
		SPAN_INSPECTOR_END()
	};
}
