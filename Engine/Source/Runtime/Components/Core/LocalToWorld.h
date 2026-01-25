#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Math/SpanMath.h"

namespace Span
{
	// レンダリングや物理演算で使用する「計算済みのワールド行列」
	struct LocalToWorld
	{
		Matrix4x4 Value;

		LocalToWorld() : Value(Matrix4x4::Identity()) {}
		LocalToWorld(const Matrix4x4& matrix) : Value(matrix) {}
	};
}
