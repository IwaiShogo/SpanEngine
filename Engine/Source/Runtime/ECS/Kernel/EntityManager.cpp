#include "EntityManager.h"

namespace Span
{
	Entity Span::EntityManager::CreateEntity()
	{
		uint32 idx;

		// 1. 再利用できるIDがあるか確認
		if (freeIndices.size() > MINIMUM_FREE_INDICES)
		{
			idx = freeIndices.front();
			freeIndices.pop_back();
		}
		else
		{
			// 2. 無ければ新規作成
			generations.push_back(0);
			idx = static_cast<uint32>(generations.size() - 1);
		}

		activeCount++;

		// EntityIDを作成して返す
		return Entity{ { idx, generations[idx] } };
	}

	void EntityManager::DestroyEntity(Entity entity)
	{
		const uint32 idx = entity.ID.Index;

		// 範囲外チェック or 既に世代が変わっている(二重削除など)なら無視
		if (idx >= generations.size() || generations[idx] != entity.ID.Generation)
		{
			SPAN_WARN("Attempted to destroy an invalid or already destroyed entity: Index %d", idx);
			return;
		}

		// 世代を進める
		generations[idx]++;

		// フリーリストに追加
		freeIndices.push_back(idx);
		activeCount--;

		// 開発用ログ
		// SPAN_LOG("Entity Destroyed: Index %d, Next Gen %d", idx, generations[idx]);
	}

	bool EntityManager::IsAlive(Entity entity) const
	{
		// 1. インデックスが範囲内か
		if (entity.ID.Index >= generations.size())
		{
			return false;
		}

		// 2. 世代が一致しているか
		return generations[entity.ID.Index] == entity.ID.Generation;
	}
}
