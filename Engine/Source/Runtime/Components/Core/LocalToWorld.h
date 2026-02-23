/*****************************************************************//**
 * @file	LocalToWorld.h
 * @brief	計算済みのワールド変換行列
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
	 * @struct	LocalToWorld
	 * @brief	🌏 ローカル座標からワールド座標への変換行列キャッシュ。
	 * 
	 * @details
	 * `TransformSystem` によって毎フレーム計算・更新されます。
	 * レンダラーや物理エンジンは、`Transform` ではなくこのコンポーネントの行列を参照します。
	 * 親子関係がある場合、親の行列も乗算された最終結果がここに格納されます。
	 */
	struct LocalToWorld
	{
		Matrix4x4 Value;

		LocalToWorld() : Value(Matrix4x4::Identity()) {}
		LocalToWorld(const Matrix4x4& matrix) : Value(matrix) {}

		SPAN_INSPECTOR_BEGIN(LocalToWorld)
			SPAN_FIELD(Value, HideInInspector())
		SPAN_INSPECTOR_END()
	};
}

