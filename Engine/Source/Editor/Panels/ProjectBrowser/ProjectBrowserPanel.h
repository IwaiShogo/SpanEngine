/*****************************************************************//**
 * @file	ProjectBrowserPanel.h
 * @brief	プロジェクトのアセット閲覧・管理を行うパネルクラスの定義。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

#include "Editor/Panels/EditorPanel.h"

namespace Span
{
	/**
	 * @class	ProjectBrowserPanel
	 * @brief	アセットディレクトリの内容を表示し、ドラッグ&ドロップ操作を提供するエディタパネル。
	 *
	 * @details
	 * **主な機能**
	 * - **2ペイン構成:** 左側にディレクトリツリー、右側にコンテンツ一覧を表示。
	 * - **アセット操作:** リネーム(F2)、削除(Delete)、複数選択(Ctrl/Shift)。
	 * - **作成メニュー:** 右クリックから C++ Component/System, Folder, Material 等を作成可能。
	 * - **ドラッグ&ドロップ:** アセットをシーンやInspectorへドラッグして割り当て可能。
	 * - **表示調整:** アイコンサイズのスライダー変更、検索フィルタリング。
	 */
	class ProjectBrowserPanel : public EditorPanel
	{
	public:
		/**
		 * @brief	コンストラクタ
		 * @param	name パネルのタイトル
		 */
		ProjectBrowserPanel();

		/**
		 * @brief	デストラクタ
		 */
		virtual ~ProjectBrowserPanel() override = default;

		/**
		 * @brief	GUI描画の更新処理
		 * @details	ImGuiの描画コマンド発行はここで行う。
		 */
		virtual void OnImGuiRender() override;

	private:
		// --- Drawing Methods ---

		/**
		 * @brief	ナビゲーションバー (戻るボタン、パンくずリスト、検索、設定) を描画する。
		 */
		void DrawNavBar();

		/**
		 * @brief	左側のディレクトリツリーを描画する。
		 */
		void DrawDirectoryTree();

		/**
		 * @brief	右側のファイル/アセット一覧エリアを描画します。
		 */
		void DrawContentArea();

		/**
		 * @brief	右クリックコンテキストメニューを描画します。
		 * @details	何もない場所でのクリックと、アイテム上でのクリックで挙動が変化します。
		 */
		void DrawContextMenu();

		/**
		 * @brief	個別のアセットアイテムを描画します。
		 * @param	entry 対象のエントリ
		 */
		void DrawEntryItem(const std::filesystem::directory_entry& entry);

		// --- Helper Methods ---

		/**
		 * @brief	指定したパスのディレクトリノードを再帰的に描画する。
		 * @param	path 描画対象のディレクトリパス
		 */
		void DrawTreeNode(const std::filesystem::path& path);

		/**
		 * @brief	指定されたパスを選択状態にします (複数選択対応)。
		 * @param	path 対象パス
		 * @param	multiSelect trueの場合、既存の選択をクリアせずに追加します。
		 */
		void SelectItem(const std::filesystem::path& path, bool multiSelect);

		/**
		 * @brief	範囲選択
		 * @param	start 最初のパス
		 * @param	end 最後のパス
		 */
		void SelectRange(const std::filesystem::path& start, const std::filesystem::path& end);

		/**
		 * @brief	テンプレートからファイルを生成します。
		 * @param	fileName 生成するファイル名
		 * @param	content ファイルの内容
		 */
		void CreateFileFromTemplate(const std::string& fileName, const std::string& content);

		/**
		 * @brief	C++ コンポーネントのテンプレートを生成します。
		 */
		void CreateComponentScript();

		/**
		 * @brief	C++ システムのテンプレートを生成します。
		 */
		void CreateSystemScript();

	private:
		// --- Paths ---
		std::filesystem::path m_BaseDirectory;				///< アセットのルートディレクトリ (Assets/)
		std::filesystem::path m_CurrentDirectory;			///< 現在表示中のディレクトリ

		// --- View Settings ---
		float m_ThumbnailSize = 96.0f;						///< アイコンの表示サイズ
		float m_Padding = 16.0f;							///< アイコン間のパディング
		std::string m_SearchFilter;							///< ファイル名検索フィルタ

		// --- Selection & Interaction ---
		std::set<std::filesystem::path> m_SelectedItems;	///< 現在選択されているアイテムのリスト
		bool m_IsRenaming = false;							///< 現在リネーム操作中のどうかのフラグ
		char m_RenameBuffer[256] = "";						///< リネーム用の入力バッファ
		std::filesystem::path m_RenamingPath;				///< リネーム対象のパス
		std::filesystem::path m_LastSelectedPath;			///< Shift選択用
		bool m_ShowDeleteDialog = false;					///< 削除時のダイアログのフラグ

		// --- Icons (Textures) ---
		// TODO: Texture* m_FolderIcon;
		// TODO: Texture* m_FileIcon;
	};
}
