/*****************************************************************//**
 * @file	PointLight.h
 * @brief	全方位を照らす光
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
	 * @struct	PointLight
	 * @brief	一点から全方位に広がり、距離によって減衰する光源。
	 */
	struct PointLight
	{
		Vector3 Color = { 1.0f, 1.0f, 1.0f };	///< 光の色
		float Intensity = 10.0f;				///< 光の強さ
		float Range = 10.0f;					///< 光が届く最大距離

		bool CastShadows = false;				///< 影を生成するか

		PointLight() = default;

		SPAN_INSPECTOR_BEGIN(PointLight)
			SPAN_FIELD(Color)
			SPAN_FIELD(Intensity, Min(0.0f))
			SPAN_FIELD(Range, Min(0.1f))
			SPAN_FIELD(CastShadows)
		SPAN_INSPECTOR_END()
	};
}
