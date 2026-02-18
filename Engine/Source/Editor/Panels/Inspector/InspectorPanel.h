/*****************************************************************//**
 * @file	InspectorPanel.h
 * @brief	エンティティの詳細情報表示パネル。
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
	 * @class	InspectorPanel
	 * @brief	🕵️ 選択中のEntityが持つコンポーネントを列挙し、編集UIを提供するパネル。
	 *
	 * @details
	 * `SelectionManager` から選択中のEntityを取得し、
	 * `ComponentRegistry` (リフレクションシステム) を使用し、
	 * そのEntityが持つ全てのコンポーネントを動的に描画します。
	 */
	class InspectorPanel : public EditorPanel
	{
	public:
		InspectorPanel() : EditorPanel("Inspector") {}

		void OnImGuiRender() override;

		/**
		 * @brief	アセット用のインスペクターを描画します。
		 * @param	paths 選択されている全てのアセットパス
		 *
		 * @details
		 * - 単一選択の場合: 詳細情報を表示します。
		 * - 複数選択の場合: 選択アイテムのリストと、Primary Asset (最後に選んだもの) の簡易プレビューを表示します。
		 */
		void DrawAssetInspector(const std::vector<std::filesystem::path>& paths);
	};
}

