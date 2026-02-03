#pragma once
#include "Reflection/SpanReflection.h"

namespace Span
{
	struct Camera
	{
		float Fov = 45.0f;		// 画角（度数法）
		float NearClip = 0.1;	// これより手前は描画しない
		float FarClip = 1000.0f;	// これより奥は描画しない

		Camera() = default;
		Camera(float fov) : Fov(fov) {}

		SPAN_INSPECTOR_BEGIN(Camera)
			SPAN_FIELD(Fov, Range(1.0f, 179.0f), Tooltip("Field of View"))
			SPAN_FIELD(NearClip, Min(0.01f), Tooltip("Cannot be 0"))
			SPAN_FIELD(FarClip, Min(0.01f), Header("Far Clip dayo"), ReadOnly())
		SPAN_INSPECTOR_END()
	};
}

