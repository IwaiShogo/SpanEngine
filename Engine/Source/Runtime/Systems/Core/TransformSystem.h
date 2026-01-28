#pragma once
#include "ECS/Kernel/System.h"
#include "ECS/Kernel/World.h"
#include "Components/Core/Transform.h"
#include "Components/Core/LocalToWorld.h"
#include "Components/Core/Relationship.h"

namespace Span
{
	class TransformSystem : public System
	{
	public:
		void OnUpdate() override
		{
			GetWorld()->ForEach<Transform, LocalToWorld>(
				[&](Entity entity, Transform& t, LocalToWorld& ltw)
				{
					// 行列計算
					ltw.Value = ComputeWorldMatrix(entity);
				}
			);
		}

	private:
		// 再帰的にワールド行列を計算するヘルパー
		Matrix4x4 ComputeWorldMatrix(Entity entity)
		{
			World* world = GetWorld();

			// 1. 自身のローカル行列 (T * R * S)
			Transform* t = world->GetComponentPtr<Transform>(entity);
			if (!t) return Matrix4x4::Identity();

			Matrix4x4 localMat = Matrix4x4::TRS(t->Position, t->Rotation, t->Scale);

			// 2. 親がいるか確認
			Parent* parent = world->GetComponentPtr<Parent>(entity);
			if (parent && !parent->Value.IsNull())
			{
				// 親のワールド行列を再帰的に取得
				Matrix4x4 parentWorldMat = ComputeWorldMatrix(parent->Value);

				// 親 * 自分
				return localMat * parentWorldMat;
			}

			return localMat;
		}
	};
}
