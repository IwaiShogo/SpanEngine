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

	private:
		/**
		 * @brief	アセット用のインスペクターを描画します。
		 */
		void DrawAssetInspector(const std::vector<std::filesystem::path>& paths);

		/**
		 * @brief	エンティティ用のインスペクターを描画します。
		 */
		void DrawEntityInspector(Entity selected, class World& world);

		/**
		 * @brief	タグのモーダル・ポップアップ
		 */
		void DrawTagEditorModal();

		/**
		 * @brief	レイヤーのモーダル・ポップアップ
		 */
		void DrawLayerEditorModal();

	private:
		bool m_OpenTagEditor = false;
		bool m_OpenLayerEditor = false;
	};
}

