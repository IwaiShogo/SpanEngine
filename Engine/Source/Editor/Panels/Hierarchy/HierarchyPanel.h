/*****************************************************************//**
 * @file	HierarchyPanel.h
 * @brief	シーン内のエンティティ階層構造表示パネル。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Editor/Panels/EditorPanel.h"
#include "ECS/Kernel/Entity.h"

namespace Span
{
	/**
	 * @class	HierarchyPanel
	 * @brief	🌳 シーン上の全エンティティをツリー形式で表示するパネル。
	 * 
	 * @details
	 * - **Tree View**: 親子関係 (`Relationship` コンポーネント) に基づいて再帰的にノードを表示します。
	 * - **Selection**: クリックで選択、Ctrlクリックで複数選択、Shiftクリックで範囲選択に対応。
	 * - **Drag & Drop**: エンティティをドラッグして他のエンティティの子にしたり、順序を入れ替えたりできます。
	 * - **Context Menu**: 右クリックでエンティティの作成・削除・複製が行えます。
	 * 
	 */
	class HierarchyPanel : public EditorPanel
	{
	public:
		HierarchyPanel() : EditorPanel("Hierarchy") {}

		void OnImGuiRender() override;

	private:
		/**
		 * @brief	1つのエンティティとその子孫を再帰的に描画します。
		 * @param	entity 描画対象のエンティティ
		 */
		void DrawEntityNode(Entity entity);

		/// @brief	エンティティ上での右クリックメニューを描画します。
		void DrawContextMenu(Entity entity);

		/// @brief	何もない場所での右クリックメニュー (新規作成など) を描画します。
		void DrawEmptySpaceContextMenu();

		/**
		 * @brief	ドラッグ&ドロップの受け入れ処理 (親子付け替え・並び替え)。
		 * @param	targetEntity ドロップ先のエンティティ
		 */
		void HandleDragDrop(Entity targetEntity);
	};
}

