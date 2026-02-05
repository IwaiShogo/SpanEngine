/*****************************************************************//**
 * @file	Camera.h
 * @brief	レンダリング視点を定義するカメラコンポーネント。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/Math/SpanMath.h"
#include "Reflection/SpanReflection.h"

namespace Span
{
	/**
	 * @struct	Camera
	 * @brief	📷 シーンを撮影するための投影設定を保持するコンポーネント。
	 * 
	 * @details
	 * 画角(FOV)やクリッピング平面などの設定を持ちます。
	 * 実際のビュー行列計算は`CameraSystem` が行い、この構造体の `ViewMatrix`, `ProjectionMatrix` に書き込みます。
	 */
	struct Camera
	{
		float Fov = 45.0f;			///< 垂直画角（度数法）
		float NearClip = 0.1;		///< 近クリップ面 (これより手前は描画しない)
		float FarClip = 1000.0f;	///< 遠クリップ面 (これより奥は描画しない)

		Camera() = default;
		Camera(float fov) : Fov(fov) {}

		// Reflection
		// ============================================================
		SPAN_INSPECTOR_BEGIN(Camera)
			SPAN_FIELD(Fov, Range(1.0f, 179.0f), Tooltip("Field of View"))
			SPAN_FIELD(NearClip, Min(0.01f), Tooltip("Cannot be 0"))
			SPAN_FIELD(FarClip, Min(0.01f), Header("Far Clip dayo"), ReadOnly())
		SPAN_INSPECTOR_END()
	};
}

