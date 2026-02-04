/*****************************************************************//**
 * @file	SelectionManager.h
 * @brief	エディタ内でのオブジェクト選択状態の管理
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "ECS/Kernel/Entity.h"

namespace Span
{
	/**
	 * @class	SelectionManager
	 * @brief	👆 選択されたEntityをグローバルに管理するクラス。
	 *
	 * @details
	 * HierarchyPanelで選択されたEntityを保持し、InspectorPanelなど他のパネルと共有します。
	 * 複数選択 (Multi-Select)に対応しています。
	 */
	class SelectionManager
	{
	public:
		/// @brief	選択リストにEntityを追加します (Ctrl+Click相当)。
		static void Add(Entity entity)
		{
			if (std::find(selections.begin(), selections.end(), entity) == selections.end())
			{
				selections.push_back(entity);
			}
		}

		/// @brief	選択リストをクリアし、指定したEntityのみを選択状態にします (Click相当)。
		static void Select(Entity entity)
		{
			selections.clear();
			selections.push_back(entity);
		}

		/// @brief	指定したEntityを選択解除します。
		static void Deselect(Entity entity)
		{
			auto it = std::remove(selections.begin(), selections.end(), entity);
			if (it != selections.end()) selections.erase(it, selections.end());
		}

		/// @brief	全ての選択を解除します。
		static void Clear()
		{
			selections.clear();
		}

		/**
		 * @brief	メインの選択対象 (最後に選択されたもの) を取得します。
		 * @return	Entityハンドル。何も選択されていない場合は `Entity::Null`。
		 */
		static Entity GetPrimary()
		{
			return selections.empty() ? Entity::Null : selections.back();
		}

		/// @brief	現在選択されている全てのEntityのリストを取得します。
		static const std::vector<Entity>& GetSelections() { return selections; }

		/// @brief	指定したEntityが選択されているか確認します。
		static bool IsSelected(Entity entity)
		{
			return std::find(selections.begin(), selections.end(), entity) != selections.end();
		}

	private:
		// 静的な選択リスト
		inline static std::vector<Entity> selections;
	};
}

