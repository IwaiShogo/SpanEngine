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
#include "Utils/EditorFileSystem.h"
#include "Editor/Commands/FileCommands.h"

#include <imgui.h>

// Fallback Icons
#ifndef ICON_FA_FOLDER
#define ICON_FA_FOLDER "[DIR]"
#endif
#ifndef ICON_FA_ARROW_LEFT
#define ICON_FA_ARROW_LEFT "<"
#endif
#ifndef ICON_FA_ARROW_RIGHT
#define ICON_FA_ARROW_RIGHT ">"
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
#ifndef ICON_FA_TRASH
#define ICON_FA_TRASH "[DEL]"
#endif

namespace Span
{
	static AutoRegisterPanel<ProjectBrowserPanel> _reg("Project Browser");

	struct BrowserHistory
	{
		std::vector<std::unique_ptr<ICommand>> UndoStack;
		std::vector<std::unique_ptr<ICommand>> RedoStack;
	};
	static BrowserHistory s_History;

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
	}

	void ProjectBrowserPanel::OnImGuiRender()
	{
		if (!ImGui::Begin("Project Browser"))
		{
			ImGui::End();
			return;
		}

		// 外部からのファイルドロップ処理
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
		{
			const auto& droppedFiles = Input::GetDroppedFiles();
			if (!droppedFiles.empty())
			{
				for (const auto& filePath : droppedFiles)
				{
					try
					{
						std::filesystem::path srcPath(filePath);
						std::filesystem::path destPath = m_CurrentDirectory / srcPath.filename();

						// コピー実行
						if (std::filesystem::exists(srcPath))
						{
							std::filesystem::copy(srcPath, destPath, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
							SPAN_LOG("Imported Asset: %s", destPath.string().c_str());
						}
					}
					catch (const std::exception& e)
					{
						SPAN_ERROR("Import Failed: %s", e.what());
					}
				}
				// 処理したらクリア
				Input::ClearDroppedFiles();
			}
		}

		// 1. キーボードショートカット処理
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		{
			HandleKeyboardInputs();
		}

		// 2. Navigation Bar
		DrawNavBar();
		ImGui::Separator();

		// 3. Main Area
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

		// マウスの戻るボタン対応
		if (ImGui::IsWindowFocused() && ImGui::IsMouseClicked(3)) GoBack();
		if (ImGui::IsWindowFocused() && ImGui::IsMouseClicked(4)) GoForward();

		DrawContentArea();
		DrawContextMenu();	// コンテキストメニュー (右クリック)

		ImGui::EndChild();
		ImGui::Columns(1);

		if (m_ShowDeleteDialog)
		{
			ImGui::OpenPopup("Delete Assets?");
		}

		if (ImGui::BeginPopupModal("Delete Assets?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to delete %d items?", (int)m_SelectedItems.size());
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Items will be moved to .Trash folder.");
			ImGui::Separator();

			if (ImGui::Button("Delete", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter))
			{
				for (const auto& item : m_SelectedItems)
				{
					auto cmd = std::make_unique<DeleteFileCommand>(item);
					ExecuteCommand(std::move(cmd));
				}
				m_SelectedItems.clear();
				m_ShowDeleteDialog = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				m_ShowDeleteDialog = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void ProjectBrowserPanel::DrawNavBar()
	{
		// Back / Forward Buttons
		if (ImGui::Button(ICON_FA_ARROW_LEFT)) GoBack();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_ARROW_RIGHT)) GoForward();
		ImGui::SameLine();

		// Undo / Redo Buttons
		if (ImGui::Button("Undo"))
		{
			PerformUndo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Redo"))
		{
			PerformRedo();
		}
		ImGui::SameLine();

		// Breadcrumbs
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
				ChangeDirectory(*it);
			}

			// 区切り文字
			if (std::next(it) != parts.rend())
			{
				ImGui::SameLine();
				ImGui::Text(">");
				ImGui::SameLine();
			}
		}

		ImGui::SameLine();

		// Undo/Redoの状態を表示
		ImGui::TextDisabled("| Undo: %d | Redo: %d", (int)s_History.UndoStack.size(), (int)s_History.RedoStack.size());

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

		// ドロップターゲット処理
		HandleDragDropTarget(path);

		if (ImGui::IsItemClicked())
		{
			ChangeDirectory(path);
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

				// 隠しファイルと、既に存在する .meta ファイル自体はスキップ
				// if (!m_ShowHiddenFiles && filename[0] == '.') continue;
				if (path.extension() == ".meta" || path.extension() == ".Trash") continue;

				// メタデータの自動生成/読み込みチェック
				if (!entry.is_directory())
				{
					AssetSerializer::LoadOrCreateMetadata(path);
				}

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

	void ProjectBrowserPanel::DrawEntryItem(const std::filesystem::directory_entry& entry)
	{
		const auto& path = entry.path();
		std::string filename = path.filename().string();
		std::string extension = path.extension().string();
		bool isDir = entry.is_directory();

		ImGui::PushID(filename.c_str());

		// --- アイコン / サムネイル処理 ---

		// AssetManagerからサムネイルを取得を試みる
		void* textureID = AssetManager::Get().GetEditorThumbnail(path);

		// フォールバックアイコンの設定
		const char* iconStr = isDir ? ICON_FA_FOLDER : ICON_FA_FILE;
		if (!isDir)
		{
			if (extension == ".cpp" || extension == ".h") iconStr = ICON_FA_FILE_CODE;
			else if (extension == ".png" || extension == ".jpg") iconStr = ICON_FA_IMAGE;
			else if (extension == ".fbx" || extension == ".obj") iconStr = ICON_FA_CUBE;
		}

		// ボタンサイズ
		ImVec2 size(m_ThumbnailSize, m_ThumbnailSize);

		// スタイル設定
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		// 選択状態の強調表示
		bool isSelected = (m_SelectedItems.find(path) != m_SelectedItems.end());
		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 0.5f));
		}

		// 描画: 画像があればImageButton、なければTextButton
		if (textureID)
		{
			ImGui::ImageButton("##thumbnail", (ImTextureID)textureID, size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1));
		}
		else
		{
			// 従来のフォントアイコン
			ImGui::Button(iconStr, size);
		}

		// フォルダアイコンへのドロップターゲット
		if (isDir)
		{
			HandleDragDropTarget(path);
		}

		// クリック・ドラッグ処理
		if (ImGui::IsItemHovered())
		{
			// シングルクリック (選択)
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				bool ctrl = ImGui::GetIO().KeyCtrl;
				bool shift = ImGui::GetIO().KeyShift;

				if (shift && !m_LastSelectedPath.empty())
				{
					// Shift範囲選択
					SelectRange(m_LastSelectedPath, path);
				}
				else
				{
					// 通常選択 / Ctrl選択
					SelectItem(path, ctrl);
					m_LastSelectedPath = path;	// 始点を記録
				}
			}
			// ダブルクリック (開く/移動)
			else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (isDir)
				{
					ChangeDirectory(path);
					m_LastSelectedPath.clear();
				}
				else
				{
					EditorFileSystem::OpenExternal(path);
				}
			}
		}

		if (isSelected) ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		// 3. Drag & Drop Source
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			// ペイロードとしてファイルの絶対パスを渡す
			const wchar_t* itemPath = path.c_str();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));

			// ドラッグ中のプレビュー表示
			if (textureID)
			{
				ImGui::Image((ImTextureID)textureID, ImVec2(64, 64));
			}
			else
			{
				ImGui::Text("%s %s", iconStr, filename.c_str());
			}

			ImGui::EndDragDropSource();
		}

		// アイテムごとの右クリックメニューをここに実装
		if (ImGui::BeginPopupContextItem("ItemContextMenu"))
		{
			// 右クリックしたアイテムが未選択なら、選択状態にする
			if (m_SelectedItems.find(path) == m_SelectedItems.end())
			{
				SelectItem(path, false);
			}

			// --- メニュー項目 ---

			// 単一選択時のリネーム可能
			if (m_SelectedItems.size() == 1)
			{
				if (ImGui::MenuItem("Rename", "F2"))
				{
					m_IsRenaming = true;
					m_RenameFocus = true;
					m_RenamingPath = path;
					strncpy_s(m_RenameBuffer, filename.c_str(), sizeof(m_RenameBuffer));
				}
			}

			if (ImGui::MenuItem("Delete", "Del"))
			{
				m_ShowDeleteDialog = true;
			}

			// TODO: 将来的に Open, Show in Explorer 等も追加

			ImGui::EndPopup();
		}

		// 4. ファイル名 / リネーム
		if (m_IsRenaming && m_RenamingPath == path)
		{
			if (m_RenameFocus)
			{
				ImGui::SetKeyboardFocusHere();
				m_RenameFocus = false;
			}

			if (ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
			{
				std::string newName = m_RenameBuffer;
				if (!newName.empty() && newName != path.filename().string())
				{
					auto cmd = std::make_unique<RenameFileCommand>(path, newName);
					ExecuteCommand(std::move(cmd));
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

	void ProjectBrowserPanel::DrawContextMenu()
	{
		if (ImGui::BeginPopupContextWindow("ProjectBrowserContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			// --- 作成メニュー ---
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Folder"))
				{
					std::filesystem::path newPath = m_CurrentDirectory / "New Folder";
					int i = 1;
					while (std::filesystem::exists(newPath))
					{
						newPath = m_CurrentDirectory / ("New Folder " + std::to_string(i++));
					}

					auto cmd = std::make_unique<CreateDirectoryCommand>(newPath);
					ExecuteCommand(std::move(cmd));
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
	}

	void ProjectBrowserPanel::HandleKeyboardInputs()
	{
		// リネーム中はショートカット無効
		if (m_IsRenaming) return;

		auto& io = ImGui::GetIO();
		bool ctrl = io.KeyCtrl;
		bool shift = io.KeyShift;

		// ウィンドウフォーカス中のみ反応
		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) return;

		// Undo: Ctrl + Z
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
		{
			// Shiftが押されていたらRedoへ
			if (shift)
			{
				PerformRedo();
			}
			else
			{
				PerformUndo();
			}
		}

		// Redo: Ctrl + Y
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
		{
			PerformRedo();
		}

		// Rename: F2
		if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
		{
			if (m_SelectedItems.size() == 1)
			{
				m_IsRenaming = true;
				m_RenameFocus = true;
				m_RenamingPath = *m_SelectedItems.begin();
				strncpy_s(m_RenameBuffer, m_RenamingPath.filename().string().c_str(), sizeof(m_RenameBuffer));
			}
		}

		// Delete: Delete Key
		if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			if (!m_SelectedItems.empty() && !m_IsRenaming)
			{
				m_ShowDeleteDialog = true;
			}
		}

		// Select All: Ctrl + A
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
		{
			m_SelectedItems.clear();
			for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				if (entry.path().filename() != ".Trash" && entry.path().extension() != ".meta")
					m_SelectedItems.insert(entry.path());
			}
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

	void ProjectBrowserPanel::SelectRange(const std::filesystem::path& start, const std::filesystem::path& end)
	{
		m_SelectedItems.clear();

		// カレントディレクトリ内のファイルを操作して、startとendの間にあるものをすべて選択する
		std::vector<std::filesystem::path> allFiles;
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				allFiles.push_back(entry.path());
			}
		}
		catch (...) { return; }

		int startIndex = -1;
		int endIndex = -1;

		for (int i = 0; i < allFiles.size(); ++i)
		{
			if (allFiles[i] == start) startIndex = i;
			if (allFiles[i] == end) endIndex = i;
		}

		if (startIndex != -1 && endIndex != -1)
		{
			// startとendが逆でも対応できるように
			int minIdx = std::min(startIndex, endIndex);
			int maxIdx = std::max(startIndex, endIndex);

			for (int i = minIdx; i <= maxIdx; ++i)
			{
				m_SelectedItems.insert(allFiles[i]);
			}
		}
	}

	void ProjectBrowserPanel::ChangeDirectory(const std::filesystem::path& newDir)
	{
		if (m_CurrentDirectory == newDir) return;

		// 履歴に追加
		m_BackHistory.push_back(m_CurrentDirectory);
		m_ForwardHistory.clear();

		m_CurrentDirectory = newDir;
		m_SelectedItems.clear();
	}

	void ProjectBrowserPanel::GoBack()
	{
		if (m_BackHistory.empty()) return;

		m_ForwardHistory.push_back(m_CurrentDirectory);
		m_CurrentDirectory = m_BackHistory.back();
		m_BackHistory.pop_back();
		m_SelectedItems.clear();
	}

	void ProjectBrowserPanel::GoForward()
	{
		if (m_ForwardHistory.empty()) return;

		m_BackHistory.push_back(m_CurrentDirectory);
		m_CurrentDirectory = m_ForwardHistory.back();
		m_ForwardHistory.pop_back();
		m_SelectedItems.clear();
	}

	void ProjectBrowserPanel::ExecuteCommand(std::unique_ptr<ICommand> command)
	{
		if (command->Execute())
		{
			SPAN_LOG("Command Executed: %s", command->GetName());
			m_UndoStack.push_back(std::move(command));
			m_RedoStack.clear();
		}
		else
		{
			SPAN_WARN("Command execution failed: %s", command->GetName());
		}
	}

	void ProjectBrowserPanel::PerformUndo()
	{
		if (!m_RedoStack.empty())
		{
			auto cmd = std::move(m_UndoStack.back());
			m_UndoStack.pop_back();
			SPAN_LOG("Undoing: %s", cmd->GetName());
			cmd->Undo();
			m_RedoStack.push_back(std::move(cmd));
		}
		else
		{
			SPAN_WARN("Undo Stack is empty");
		}
	}

	void ProjectBrowserPanel::PerformRedo()
	{
		if (!m_RedoStack.empty())
		{
			auto cmd = std::move(m_RedoStack.back());
			m_RedoStack.pop_back();
			SPAN_LOG("Redoing: %s", cmd->GetName());
			cmd->Execute();
			m_UndoStack.push_back(std::move(cmd));
		}
	}

	void ProjectBrowserPanel::HandleDragDropTarget(const std::filesystem::path& targetPath)
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* sourcePathStr = (const wchar_t*)payload->Data;
				std::filesystem::path sourcePath(sourcePathStr);

				// 自分自身や、自分のサブフォルダへの移動を防ぐ
				bool isSelf = (sourcePath == targetPath);
				bool isSubDir = false;
				if (targetPath.string().find(sourcePath.string()) == 0) isSubDir = true;

				if (!isSelf && !isSubDir)
				{
					auto cmd = std::make_unique<MoveFileCommand>(sourcePath, targetPath);
					ExecuteCommand(std::move(cmd));
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void ProjectBrowserPanel::CreateFileFromTemplate(const std::string& fileName, const std::string& content)
	{
		std::filesystem::path path = m_CurrentDirectory / fileName;

		// 重複チェック
		int i = 1;
		while (std::filesystem::exists(path))
		{
			path = m_CurrentDirectory / (path.stem().string() + "_" + std::to_string(i++) + path.extension().string());
		}

		auto cmd = std::make_unique<CreateFileCommand>(path, content);
		ExecuteCommand(std::move(cmd));
	}

	void ProjectBrowserPanel::CreateComponentScript()
	{
		// 簡易テンプレート
		std::string name = "NewComponent";
		std::string filename = name + ".h";

		std::string content = R"(#pragma once
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

		std::string content = R"(#pragma once
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
