#pragma once
#include "Core/Math/SpanMath.h" 

namespace Span
{
	struct Transform
	{
		Vector3 Position	= { 0.0f, 0.0f, 0.0f };
		Vector3 Rotation	= { 0.0f, 0.0f, 0.0f };
		Vector3 Scale		= { 1.0f, 1.0f, 1.0f };

		Matrix4x4 GetWorldMatrix() const
		{
			XMMATRIX mScale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
			XMMATRIX mRot = XMMatrixRotationRollPitchYaw(
				ToRadians(Rotation.x), ToRadians(Rotation.y), ToRadians(Rotation.z));
			XMMATRIX mTrans = XMMatrixTranslation(Position.x, Position.y, Position.z);

			Matrix4x4 result;
			result.FromXM(mScale * mRot * mTrans);
			return result;
		}
	};
}