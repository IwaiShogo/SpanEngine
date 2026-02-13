/*****************************************************************//**
 * @file	ProjectBrowserPanel.cpp
 * @brief	ProjectBrowserPanelの実装。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include <SpanEngine.h>
#include <SpanEditor.h>
#include "ProjectBrowserPanel.h"

#include <imgui.h>
//#include <imgui_stdlib.h>

// Fallback Icons
#ifndef ICON_FA_FOLDER
#define ICON_FA_FOLDER "[DIR]"
#endif
#ifndef ICON_FA_FILE
#define ICON_FA_FILE "[FILE]"
#endif
#ifndef ICON_FA_FILE_CODE
#define ICON_FA_FILE_CODE "[CODE]"
#endif
#ifndef ICON_FA_IMAGE
#define ICON_FA_IMAGE "[IMG]"
#endif
#ifndef ICON_FA_CUBE
#define ICON_FA_CUBE "[MESH]"
#endif

namespace Span
{
	static AutoRegisterPanel<ProjectBrowserPanel> _reg("Project Browser");

	ProjectBrowserPanel::ProjectBrowserPanel()
		: EditorPanel("Project Browser")
	{
		// TODO: プロジェクトパスを動的に取得する仕組み
		m_BaseDirectory = "../Projects/Playground/Assets";
		m_CurrentDirectory = m_BaseDirectory;

		// ディレクトリが存在しない場合のケア
		if (!std::filesystem::exists(m_BaseDirectory))
		{
			std::filesystem::create_directories(m_BaseDirectory);
		}

		// アイコンロード(将来的)
		// m_FolderIcon = Texture::Create("Resources/Icons/Folder.png");
	}

	void ProjectBrowserPanel::OnImGuiRender()
	{
		if (!ImGui::Begin("Project Browser"))
		{
			ImGui::End();
			return;
		}

		// 1. Navigation Bar
		DrawNavBar();
		ImGui::Separator();

		// 2. Main Area
		// 2カラムレイアウト (左: ツリー、右: コンテンツ)
		static float leftPaneWidth = 200.0f;
		ImGui::Columns(2, "ProjectBrowserColumns", true);

		// 初回のみ幅を設定
		if (ImGui::GetFrameCount() == 1)
		{
			ImGui::SetColumnWidth(0, leftPaneWidth);
		}

		// --- Left Pane: Directory Tree ---
		ImGui::BeginChild("DirectoryTree");
		DrawDirectoryTree();
		ImGui::EndChild();

		ImGui::NextColumn();

		// --- Right Pane: Content Area ---
		ImGui::BeginChild("ContentArea");

		// 戻るボタンのショートカット (Backspace)
		if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Backspace))
		{
			if (m_CurrentDirectory != m_BaseDirectory)
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		DrawContentArea();
		DrawContextMenu();	// コンテキストメニュー (右クリック)

		ImGui::EndChild();

		ImGui::Columns(1);

		ImGui::End();
	}

	void ProjectBrowserPanel::DrawNavBar()
	{
		// パンくずリスト
		std::vector<std::filesystem::path> parts;
		for (auto p = m_CurrentDirectory; p != m_BaseDirectory; p = p.parent_path())
		{
			parts.push_back(p);
		}
		parts.push_back(m_BaseDirectory); // Root

		// 逆順で描画
		for (auto it = parts.rbegin(); it != parts.rend(); ++it)
		{
			std::string name = (it->filename() == "Assets") ? "Assets" : it->filename().string();

			// パスボタン
			if (ImGui::Button(name.c_str()))
			{
				m_CurrentDirectory = *it;
			}

			// 区切り文字
			if (std::next(it) != parts.rend())
			{
				ImGui::SameLine();
				ImGui::Text(">");
				ImGui::SameLine();
			}
		}

		// 右寄せグループ
		float searchWidth = 150.0f;
		float sliderWidth = 100.0f;
		float spacing = 10.0f;
		float rightAreaWidth = searchWidth + sliderWidth + spacing * 2;

		ImGui::SameLine();
		float availX = ImGui::GetContentRegionAvail().x;
		if (availX > rightAreaWidth)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availX - rightAreaWidth);
		}

		// 検索バー
		ImGui::PushItemWidth(searchWidth);
		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strncpy_s(buffer, m_SearchFilter.c_str(), sizeof(buffer));
		if (ImGui::InputTextWithHint("##Search", "Search...", buffer, sizeof(buffer)))
		{
			m_SearchFilter = std::string(buffer);
		}

		ImGui::PopItemWidth();

		ImGui::SameLine();

		// ズームスライダー
		ImGui::PushItemWidth(sliderWidth);
		ImGui::SliderFloat("##Zoom", &m_ThumbnailSize, 32.0f, 128.0f, "");
		ImGui::PopItemWidth();
	}

	void ProjectBrowserPanel::DrawDirectoryTree()
	{
		DrawTreeNode(m_BaseDirectory);
	}

	void ProjectBrowserPanel::DrawContentArea()
	{
		float cellSize = m_ThumbnailSize + m_Padding;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, 0, false);

		// ディレクトリ走査
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				const auto& path = entry.path();
				std::string filename = path.filename().string();

				// 隠しファイルスキップ
				// if (!m_ShowHiddenFiles && filename[0] == '.') continue;

				// 検索フィルタ適用
				if (!m_SearchFilter.empty())
				{
					// TODO: 大文字小文字を無視した部分一致などを実装
					if (filename.find(m_SearchFilter) == std::string::npos) continue;
				}

				DrawEntryItem(entry);
				ImGui::NextColumn();
			}
		}
		catch (const std::exception& e)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Access Error: %s", e.what());
		}

		ImGui::Columns(1);

		// 空白クリックで選択解除
		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
		{
			m_SelectedItems.clear();
			m_IsRenaming = false;
		}
	}

	void ProjectBrowserPanel::DrawContextMenu()
	{
		if (ImGui::BeginPopupContextWindow("ProjectBrowserContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			// --- 作成メニュー ---
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Folder"))
				{
					std::filesystem::create_directory(m_CurrentDirectory / "New Folder");
				}
				ImGui::Separator();
				if (ImGui::MenuItem("C++ Component", "Struct"))
				{
					CreateComponentScript();
				}
				if (ImGui::MenuItem("C++ System", "System"))
				{
					CreateSystemScript();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Material", ".mat"))
				{
					// TODO: Create Material Asset
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Show in Explorer"))
			{
				// Windows固有: エクスプローラーで開く
				system(("explorer" + m_CurrentDirectory.string()).c_str());
			}

			ImGui::EndPopup();
		}

		// アイテム上のコンテキストメニュー (リネーム・削除)
		if (!m_SelectedItems.empty() && ImGui::BeginPopupContextItem("ItemContext"))
		{
			// 単一選択時のみリネーム可能
			if (m_SelectedItems.size() == 1)
			{
				if (ImGui::MenuItem("Rename", "F2"))
				{
					m_IsRenaming = true;
					m_RenamingPath = *m_SelectedItems.begin();
					std::string filename = m_RenamingPath.filename().string();
					strcpy_s(m_RenameBuffer, filename.c_str());
				}
			}

			if (ImGui::MenuItem("Delete", "Delete"))
			{
				// 確認ダイアログを出すべき
				for (const auto& item : m_SelectedItems)
				{
					std::filesystem::remove_all(item);
				}
				m_SelectedItems.clear();
			}

			ImGui::EndPopup();
		}
	}

	void ProjectBrowserPanel::DrawEntryItem(const std::filesystem::directory_entry& entry)
	{
		const auto& path = entry.path();
		std::string filename = path.filename().string();
		std::string extension = path.extension().string();
		bool isDir = entry.is_directory();

		ImGui::PushID(filename.c_str());

		// 1. アイコン選択
		const char* iconStr = isDir ? ICON_FA_FOLDER : ICON_FA_FILE;
		if (!isDir)
		{
			if (extension == ".cpp" || extension == ".h") iconStr = ICON_FA_FILE_CODE;
			else if (extension == ".png" || extension == ".jpg") iconStr = ICON_FA_IMAGE;
			else if (extension == ".fbx" || extension == ".obj") iconStr = ICON_FA_CUBE;
		}

		// 2. ボタン (アイコン) 描画
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		// 選択状態の強調表示
		bool isSelected = (m_SelectedItems.find(path) != m_SelectedItems.end());
		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 0.5f));
		}

		// ボタン押下処理
		if (ImGui::Button(iconStr, ImVec2(m_ThumbnailSize, m_ThumbnailSize)))
		{
			bool ctrl = ImGui::GetIO().KeyCtrl;
			bool shift = ImGui::GetIO().KeyShift;	// TODO: Shift範囲選択

			SelectItem(path, ctrl);
		}

		// ダブルクリック処理
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (isDir)
			{
				m_CurrentDirectory = path;
				m_SelectedItems.clear();
			}
			else
			{
				// TODO: ファイルを開く処理 (ShellExecute?)
			}
		}

		if (isSelected) ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		// 3. Drag & Drop Source
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			// ペイロードとしてファイルの絶対パスを渡す
			std::wstring pathStr = entry.path().wstring();
			ImGui::SetDragDropPayload("PROJECT_BROWSER_ITEM", pathStr.c_str(), (pathStr.length() + 1) * sizeof(wchar_t));

			// ドラッグ中のプレビュー
			ImGui::Text("%s %s", iconStr, filename.c_str());
			ImGui::EndDragDropSource();
		}

		// 4. ファイル名 / リネーム
		if (m_IsRenaming && m_RenamingPath == path)
		{
			// リネーム入力フィールド
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
			{
				// リネーム実行
				std::filesystem::path newPath = path.parent_path() / m_RenameBuffer;
				try
				{
					std::filesystem::rename(path, newPath);
				}
				catch (std::exception& e)
				{
					// エラーログ述力 (TODO: Console Panel)
				}
				m_IsRenaming = false;
			}

			// フォーカスが外れたらキャンセル
			if (!ImGui::IsItemActivated() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1)))
			{
				m_IsRenaming = false;
			}
		}
		else
		{
			// 通常表示
			ImGui::TextWrapped("%s", filename.c_str());
		}

		ImGui::PopID();
	}

	void ProjectBrowserPanel::DrawTreeNode(const std::filesystem::path& path)
	{
		std::string filename = path.filename().string();
		if (filename.empty()) filename = path.string();

		// ツリーノードの設定
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

		if (path == m_CurrentDirectory)
			flags |= ImGuiTreeNodeFlags_Selected;

		// ディレクトリ内にサブディレクトリがあるかチェック
		bool hasSubDirs = false;
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(path))
			{
				if (entry.is_directory())
				{
					hasSubDirs = true;
					break;
				}
			}
		}
		catch (...) {}

		if (!hasSubDirs) flags |= ImGuiTreeNodeFlags_Leaf;

		// ノード描画
		bool opened = ImGui::TreeNodeEx(path.string().c_str(), flags, "%s %s", ICON_FA_FOLDER, filename.c_str());

		if (ImGui::IsItemClicked())
		{
			m_CurrentDirectory = path;
		}

		// 再帰処理
		if (opened)
		{
			if (hasSubDirs)
			{
				for (const auto& entry : std::filesystem::directory_iterator(path))
				{
					if (entry.is_directory())
					{
						DrawTreeNode(entry.path());
					}
				}
			}
			ImGui::TreePop();
		}
	}

	void ProjectBrowserPanel::SelectItem(const std::filesystem::path& path, bool multiSelect)
	{
		if (!multiSelect)
		{
			m_SelectedItems.clear();
		}

		if (m_SelectedItems.find(path) != m_SelectedItems.end())
		{
			if (multiSelect) m_SelectedItems.erase(path);
		}
		else
		{
			m_SelectedItems.insert(path);
		}
	}

	void ProjectBrowserPanel::CreateFileFromTemplate(const std::string& fileName, const std::string& content)
	{
		std::filesystem::path path = m_CurrentDirectory / fileName;

		// 重複チェック
		int i = 1;
		while (std::filesystem::exists(path))
		{
			std::string stem = path.stem().string();
			std::string ext = path.extension().string();
			path = m_CurrentDirectory / (stem + "_" + std::to_string(i) + ext);
			i++;
		}

		std::ofstream ofs(path);
		if (ofs)
		{
			ofs << content;
			ofs.close();
		}
	}

	void ProjectBrowserPanel::CreateComponentScript()
	{
		// 簡易テンプレート
		std::string name = "NewComponent";
		std::string filename = name + ".h";

		std::string content = R"(
#pragma once
#include <SpanEngine.h>

namespace App
{
	struct )" + name + R"(
	{
		flaot Value = 0.0f;
	};
}
)";
		CreateFileFromTemplate(filename, content);
	}

	void ProjectBrowserPanel::CreateSystemScript()
	{
		std::string name = "NewSystem";
		std::string filename = name + ".h";

		std::string content = R"(
#pragma once
#include <SpanEngine.h>

namespace App
{
	class )" + name + R"( : public Span::System
	{
	public:
		void OnUpdate(float dt) override
		{
			// System Logic Here
		}
	};
}
)";
		CreateFileFromTemplate(filename, content);
	}
}
