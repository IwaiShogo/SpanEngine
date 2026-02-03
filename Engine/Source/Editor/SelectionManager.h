#pragma once
#include "ECS/Kernel/Entity.h"

namespace Span
{
	class SelectionManager
	{
	public:
		// 選択の追加
		static void Add(Entity entity)
		{
			if (std::find(selections.begin(), selections.end(), entity) == selections.end())
			{
				selections.push_back(entity);
			}
		}

		// 単一選択
		static void Select(Entity entity)
		{
			selections.clear();
			selections.push_back(entity);
		}

		static void Deselect(Entity entity)
		{
			auto it = std::remove(selections.begin(), selections.end(), entity);
			if (it != selections.end()) selections.erase(it, selections.end());
		}

		static void Clear()
		{
			selections.clear();
		}

		// メインの選択対象
		static Entity GetPrimary()
		{
			return selections.empty() ? Entity::Null : selections.back();
		}

		// 全選択リスト
		static const std::vector<Entity>& GetSelections() { return selections; }

		static bool IsSelected(Entity entity)
		{
			return std::find(selections.begin(), selections.end(), entity) != selections.end();
		}

	private:
		inline static std::vector<Entity> selections;
	};
}

