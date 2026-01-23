#pragma once
#include "Entity.h"

namespace Span
{
	class EntityManager
	{
	public:
		EntityManager() = default;
		~EntityManager() = default;

		SPAN_NON_COPYABLE(EntityManager);

		// 新しいEntityを作成して返す
		Entity CreateEntity();

		// Entityを削除する
		void DestroyEntity(Entity entity);

		// そのEntityがまだ存在しているか確認する
		bool IsAlive(Entity entity) const;

		// 現在のアクティブなEntity数
		size_t GetActiveEntityCount() const { return activeCount; }

	private:
		// 各スロットの現在の世代番号を管理する配列
		std::vector<uint32> generations;

		// 再利用待ちのインデックスリスト
		std::deque<uint32> freeIndices;

		// 生存数
		size_t activeCount = 0;

		// 最低限のインデックス管理数
		static constexpr uint32 MINIMUM_FREE_INDICES = 1024;
	};
}
