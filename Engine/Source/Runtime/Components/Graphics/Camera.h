#pragma once

namespace Span
{
	struct Camera
	{
		float Fov = 45.0f;		// 画角（度数法）
		float NearClip = 0.1;	// これより手前は描画しない
		float FarClip = 100.0f;	// これより奥は描画しない
	};
}
