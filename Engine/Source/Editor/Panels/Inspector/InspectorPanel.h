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
#include "Editor/SelectionManager.h"
#include "Editor/Panels/EditorPanel.h"
#include "ECS/Kernel/Entity.h"
#include "MaterialPreviewer.h"

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
		 * @brief	マテリアルアセット用のプロパティエディタを描画します。
		 *
		 * @details
		 * ファイルパスからマテリアルデータを一時的に読み込み、ImGuiを通じて
		 * PBRパラメータの編集UIを提供します。
		 * 値が変更された場合、自動的に .mat ファイルへ上書き保存します。
		 *
		 * @param	path 選択されている .mat ファイルのパス
		 */
		void DrawMaterialEditor(const std::filesystem::path& path);

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

		// ロック機能
		bool m_IsLocked = false;
		SelectionType m_LockedType = SelectionType::None;
		std::vector<std::filesystem::path> m_LockedAssets;
		Entity m_LockedEntity = Entity::Null;

		MaterialPreviewer m_MaterialPreviewer;
	};
}

