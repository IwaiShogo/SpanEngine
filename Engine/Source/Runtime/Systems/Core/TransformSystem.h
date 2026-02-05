/*****************************************************************//**
 * @file	TransformSystem.h
 * @brief	ローカル座標からワールド座標への変換行列計算
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "ECS/Kernel/System.h"
#include "ECS/Kernel/World.h"

// Components
#include "Components/Core/Transform.h"
#include "Components/Core/LocalToWorld.h"
#include "Components/Core/Relationship.h"

namespace Span
{
	/**
	 * @class	TransformSystem
	 * @brief	🌏 全エンティティのワールド変換行列を更新するシステム。
	 *
	 * @details
	 * `Transform` (Local) と `Relationship` (親子関係) を読み取り、
	 * 最終的な `LocalToWorld` 行列を計算して書き込みます。
	 *
	 * ### 🧮 計算式
	 * \f$ M_{world} = M_{parent\_world} \times M_{local} \f$
	 */
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
		/**
		 * @brief	再帰的に親のワールド行列を取得し、自信のローカル行列と合成します。
		 * @note
		 * 現在の実装は単純な再帰呼び出しです。
		 * 階層が深い場合やオブジェクト数が多い場合、計算結果のキャッシュや
		 * 階層順のソート (Dirty Flag方式) による最適化が将来的に必要になります。
		 */
		Matrix4x4 ComputeWorldMatrix(Entity entity)
		{
			World* world = GetWorld();

			// 1. 自身のローカル行列 (T * R * S)
			Transform* t = world->GetComponentPtr<Transform>(entity);
			if (!t) return Matrix4x4::Identity();

			Matrix4x4 localMat = Matrix4x4::TRS(t->Position, t->Rotation, t->Scale);

			// 2. 親がいるか確認
			if (Relationship* rel = world->GetComponentPtr<Relationship>(entity))
			{
				if (!rel->Parent.IsNull())
				{
					Matrix4x4 parentWorldMat = ComputeWorldMatrix(rel->Parent);

					return localMat * parentWorldMat;
				}
			}

			return localMat;
		}
	};
}

