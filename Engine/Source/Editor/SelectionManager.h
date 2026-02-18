/*****************************************************************//**
 * @file	SelectionManager.h
 * @brief	エディタ内でのオブジェクト選択状態の管理
 *
 * @details
 * HierarchyPanelでのEntity選択、ProjectBrowserでのAssetファイル選択を統合管理します。
 * EntityとAssetの選択は排他的（どちらか一方のみアクティブ）であり、
 * どちらも複数選択 (Multi-Select) をサポートしています。
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
	 * @enum	SelectionType
	 * @brief	現在選択されているオブジェクトの種類を識別します。
	 */
	enum class SelectionType
	{
		None = 0,	///< 何も選択されていない
		Entity,		///< シーン上のEntityが選択されている
		Asset		///< ProjectBrowser上のアセットファイルが選択されている
	};

	/**
	 * @class	SelectionManager
	 * @brief	👆 選択されたEntityをグローバルに管理するクラス。
	 *
	 * @details
	 * 選択状態の変更（追加、削除、クリア）と、現在の選択状態へのアクセスを提供します。
	 * ProjectBrowserやInspectorPanelなど、各エディタパネル間の連携に使用されます。
	 */
	class SelectionManager
	{
	public:
		// Entity Selection
		// ============================================================

		/**
		 * @brief	選択リストにEntityを追加します (Ctrl+Click担当)。
		 * @details
		 * 既に選択されている場合は無視されます。
		 * アセットが選択されていた場合、その選択は解除され、選択タイプは `Entity` に切り替わります。
		 *
		 * @param	entity 追加するEntityハンドル
		 */
		static void Add(Entity entity)
		{
			if (currentType == SelectionType::Asset) ClearAssetSelection();
			currentType = SelectionType::Entity;

			if (std::find(entitySelections.begin(), entitySelections.end(), entity) == entitySelections.end())
			{
				entitySelections.push_back(entity);
			}
		}

		/**
		 * @brief	選択リストをクリアし、指定したEntityのみを選択状態にします (Click担当)。
		 * @param	entity 選択するEntityハンドル
		 */
		static void Select(Entity entity)
		{
			ClearAssetSelection();
			currentType = SelectionType::Entity;

			entitySelections.clear();
			entitySelections.push_back(entity);
		}

		/**
		 * @brief	指定したEntityを選択解除します。
		 * @details
		 * 選択リストから削除され、リストが空になった場合は選択タイプが `None` になります。
		 *
		 * @param	entity 解除するEntityハンドル
		 */
		static void Deselect(Entity entity)
		{
			if (currentType != SelectionType::Entity) return;

			auto it = std::remove(entitySelections.begin(), entitySelections.end(), entity);
			if (it != entitySelections.end()) entitySelections.erase(it, entitySelections.end());

			if (entitySelections.empty()) currentType = SelectionType::None;
		}

		/**
		 * @brief	指定したEntityが選択されているか確認します。
		 * @param	entity 確認対象のEntity。
		 * @return	true 選択されている場合。
		 */
		static bool IsSelected(Entity entity)
		{
			if (currentType != SelectionType::Entity) return false;
			return std::find(entitySelections.begin(), entitySelections.end(), entity) != entitySelections.end();
		}

		// Asset Selection Methods
		// ============================================================

		/**
		 * @brief	選択リストにAssetファイルを追加します (Ctrl+Click担当)。
		 * @param	path 追加するAssetのファイルパス。
		 * @details
		 * 既に選択されている場合は何もしません。
		 * 現在のモードがEntity選択だった場合、Entity選択はクリアされAsset選択モードに切り替わります。
		 */
		static void AddAsset(const std::filesystem::path& path)
		{
			if (currentType == SelectionType::Entity) ClearEntitySelection();
			currentType = SelectionType::Asset;

			if (std::find(assetSelections.begin(), assetSelections.end(), path) == assetSelections.end())
			{
				assetSelections.push_back(path);
			}
		}

		/**
		 * @brief	選択リストをクリアし、指定したAssetのみを選択状態にします (Click担当)。
		 * @param	path 選択するAssetのファイルパス。
		 */
		static void SelectAsset(const std::filesystem::path& path)
		{
			ClearEntitySelection();
			currentType = SelectionType::Asset;

			assetSelections.clear();
			assetSelections.push_back(path);
		}

		/**
		 * @brief	指定したAssetを選択解除します。
		 * @param	path 解除するAssetのファイルパス。
		 */
		static void DeselectAsset(const std::filesystem::path& path)
		{
			if (currentType != SelectionType::Asset) return;

			auto it = std::remove(assetSelections.begin(), assetSelections.end(), path);
			if (it != assetSelections.end()) assetSelections.erase(it, assetSelections.end());

			if (assetSelections.empty()) currentType = SelectionType::None;
		}

		/**
		 * @brief	指定したAssetが選択されているか確認します。
		 * @param	path 確認対象のAssetファイルパス。
		 * @return	true 選択されている場合。
		 */
		static bool IsAssetSelected(const std::filesystem::path& path)
		{
			if (currentType != SelectionType::Asset) return false;
			return std::find(assetSelections.begin(), assetSelections.end(), path) != assetSelections.end();
		}

		// Global Control & Getters
		// ============================================================

		/**
		 * @brief	全ての選択 (Entity及びAsset) を解除します。
		 */
		static void Clear()
		{
			currentType = SelectionType::None;
			entitySelections.clear();
			assetSelections.clear();
		}

		/**
		 * @brief	現在の選択タイプを取得します。
		 * @return	SelectionType (None, Entity, Asset)
		 */
		static SelectionType GetType() { return currentType; }

		/**
		 * @brief	メインのEntity選択対象（最後に選択されたもの）を取得します。
		 * @return	Entityハンドル。何も選択されていない場合は `Entity::Null`。
		 */
		static Entity GetPrimaryEntity()
		{
			return (currentType == SelectionType::Entity && !entitySelections.empty()) ? entitySelections.back() : Entity::Null;
		}

		/**
		 * @brief	メインのAsset選択 (最後に選択されたもの) を取得します。
		 * @return	Assetパス。選択されていない場合は空のパス。
		 */
		static std::filesystem::path GetPrimaryAsset()
		{
			return (currentType == SelectionType::Asset && !assetSelections.empty()) ? assetSelections.back() : std::filesystem::path();
		}

		/**
		 * @brief	現在選択されている全てのEntityリストを取得します。
		 * @return	Entityのベクタ配列への参照。
		 */
		static const std::vector<Entity>& GetEntitySelections() { return entitySelections; }

		/**
		 * @brief	現在選択されている全てのAssetリストを取得します。
		 * @return	Assetパスのベクタ配列への参照。
		 */
		static const std::vector<std::filesystem::path>& GetAssetSelections() { return assetSelections; }

	private:
		static void ClearEntitySelection() { entitySelections.clear(); }
		static void ClearAssetSelection() { assetSelections.clear(); }

	private:
		/// @brief	現在の選択モード
		inline static SelectionType currentType = SelectionType::None;
		/// @brief	選択されたEntityリスト (Entityモード時のみ有効)
		inline static std::vector<Entity> entitySelections;
		/// @brief	選択されたアセットパス (Assetモード時のみ有効)
		inline static std::vector<std::filesystem::path> assetSelections;
	};
}

