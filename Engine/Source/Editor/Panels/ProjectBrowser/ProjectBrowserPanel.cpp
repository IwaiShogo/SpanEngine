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
#define ICON_FA_TRASH "[TRASH]"
#endif
#ifndef ICON_FA_SEARCH
#define ICON_FA_SEARCH "[SEARCH]"
#endif
#ifndef ICON_FA_EYE
#define ICON_FA_EYE "[SHOW]"
#endif

namespace Span
{
	static AutoRegisterPanel<ProjectBrowserPanel> _reg("Project Browser");

	// 検索条件に合致するか判定するヘルパー
	bool ProjectBrowserPanel::MatchesFilter(const std::filesystem::path& path)
	{
		// 1. .meta は常に除外
		if (path.extension() == ".meta") return false;

		// .Trash フォルダは、Assets直下にある場合のみ隠す
		if (path.filename() == ".Trash" && m_CurrentDirectory != path.parent_path()) return false;
		if (path.filename() == ".Trash") return false;

		std::string filename = path.filename().string();

		// 2. テキスト検索
		if (!m_SearchFilter.empty())
		{
			auto it = std::search(
				filename.begin(), filename.end(),
				m_SearchFilter.begin(), m_SearchFilter.end(),
				[](char a, char b) { return std::tolower(a) == std::tolower(b); }
			);
			if (it == filename.end()) return false;
		}

		// 3. タイプフィルター
		if (m_TypeFilterIndex > 0)
		{
			std::string ext = path.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			switch (m_TypeFilterIndex)
			{
			case 1:	// Texture
				if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".tga" && ext != ".bmp") return false;
				break;
			case 2:	// Mesh
				if (ext != ".fbx" && ext != ".obj" && ext != ".gltf" && ext != ".glb") return false;
				break;
			case 3:	// Script
				if (ext != ".h" && ext != ".cpp" && ext != ".cs") return false;
				break;
			case 4:	// Material
				if (ext != ".mat") return false;
				break;
			case 5:
				if (ext != ".span") return false;
				break;
			}
		}

		return true;
	}

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

		// DirectoryWatcher
		m_DirectoryWatcher = std::make_unique<DirectoryWatcher>(m_CurrentDirectory, [this]()
		{
			m_NeedsRefresh = true;
		});
	}

	void ProjectBrowserPanel::OnImGuiRender()
	{
		if (!ImGui::Begin("Project Browser"))
		{
			ImGui::End();
			return;
		}

		// ホットリロード要求の処理
		if (m_NeedsRefresh)
		{
			// 削除されたファイルが選択リストに残っていれば選択解除
			for (auto it = m_SelectedItems.begin(); it != m_SelectedItems.end(); )
			{
				if (!std::filesystem::exists(*it))
				{
					SelectionManager::DeselectAsset(*it);
					it = m_SelectedItems.erase(it);
				}
				else
				{
					++it;
				}

				m_NeedsRefresh = false;
			}
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
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsWindowHovered(ImGuiFocusedFlags_RootAndChildWindows))
		{
			HandleKeyboardInputs();
		}

		// 2. Navigation Bar
		DrawNavBar();
		ImGui::Separator();

		// 3. Main Area
		// 2カラムレイアウト (左: ツリー、右: コンテンツ)
		static float leftPaneWidth = 200.0f;
		float bottomBarHeight = 30.0f;
		float contentHeight = ImGui::GetContentRegionAvail().y - bottomBarHeight - 10.0f;

		ImGui::Columns(2, "ProjectBrowserColumns", true);
		if (ImGui::GetFrameCount() == 1) ImGui::SetColumnWidth(0, leftPaneWidth);

		// --- Left Pane: Directory Tree ---
		ImGui::BeginChild("DirectoryTree", ImVec2(0, contentHeight));
		DrawDirectoryTree();
		ImGui::EndChild();

		ImGui::NextColumn();

		// --- Right Pane: Content Area ---
		ImGui::BeginChild("ContentArea", ImVec2(0, contentHeight));

		// マウスナビゲーション
		if (ImGui::IsWindowFocused() && ImGui::IsMouseClicked(3)) GoBack();
		if (ImGui::IsWindowFocused() && ImGui::IsMouseClicked(4)) GoForward();

		// マウスホイールズーム (Ctrl + Wheel)
		if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl)
		{
			float wheel = ImGui::GetIO().MouseWheel;
			if (wheel != 0.0f)
			{
				m_ThumbnailSize += wheel * 5.0f;
				if (m_ThumbnailSize < 32.0f) m_ThumbnailSize = 32.0f;
				if (m_ThumbnailSize > 256.0f) m_ThumbnailSize = 256.0f;
			}
		}

		DrawContentArea();
		DrawContextMenu();	// コンテキストメニュー (右クリック)

		ImGui::EndChild();
		ImGui::Columns(1);

		// --- Bottom Bar ---
		ImGui::Separator();
		ImGui::BeginChild("BottomBar", ImVec2(0, bottomBarHeight), false, ImGuiWindowFlags_NoScrollbar);
		{
			// Item Count
			int itemCount = 0;
			try
			{
				for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
				{
					if (MatchesFilter(entry.path())) itemCount++;
				}
			}
			catch (...) {}

			ImGui::AlignTextToFramePadding();
			ImGui::Text("%d items", itemCount);

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();
			float contentHeight = ImGui::GetContentRegionAvail().y - bottomBarHeight - 10.0f;

			ImGui::SameLine();

			// Zoom Slider
			float sliderWidth = 150.0f;
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - sliderWidth - 10.0f);
			ImGui::SetNextItemWidth(sliderWidth);
			ImGui::SliderFloat("##Zoom", &m_ThumbnailSize, 32.0f, 256.0f, "Zoom: %.0f");
		}
		ImGui::EndChild();

		// --- Delete Dialog
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
		// Controls
		if (ImGui::Button(ICON_FA_ARROW_LEFT)) GoBack();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_ARROW_RIGHT)) GoForward();
		ImGui::SameLine();

		// Breadcrumbs
		float footerWidth = 350.0f;
		float availWidth = ImGui::GetContentRegionAvail().x;
		float pathWidth = availWidth - footerWidth;
		if (pathWidth < 100.0f) pathWidth = 100.0f;

		ImGui::BeginChild("##PathBar", ImVec2(pathWidth, 24.0f), false, ImGuiWindowFlags_NoScrollbar);
		{
			std::vector<std::filesystem::path> parts;
			std::filesystem::path p = m_CurrentDirectory;
			std::filesystem::path trashPath = m_BaseDirectory.parent_path() / ".Trash";

			while (true)
			{
				parts.push_back(p);

				// Assetsフォルダに到達したら終了
				if (p == m_BaseDirectory) break;

				// Trashフォルダに到達したら終了
				if (p == trashPath) break;

				// ルートまで行ったら終了
				if (!p.has_parent_path() || p.parent_path() == p) break;

				p = p.parent_path();
			}

			for (auto it = parts.rbegin(); it != parts.rend(); ++it)
			{
				std::string name;
				if (*it == m_BaseDirectory) name = "Assets";
				else if (*it == trashPath) name = "Trash";	// Trash表示
				else name = it->filename().string();

				if (ImGui::Button(name.c_str())) ChangeDirectory(*it);
				if (std::next(it) != parts.rend())
				{
					ImGui::SameLine();
					ImGui::Text(">");
					ImGui::SameLine();
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Search & Filter & Tools

		// Type Filter
		ImGui::SetNextItemWidth(100.0f);
		const char* filterTypes[] = { "All", "Texture", "Mesh", "Script", "Material", "Scene" };
		ImGui::Combo("##TypeFilter", &m_TypeFilterIndex, filterTypes, IM_ARRAYSIZE(filterTypes));

		ImGui::SameLine();

		// Search Bar
		ImGui::SetNextItemWidth(150.0f);
		char buffer[256];
		strcpy_s(buffer, m_SearchFilter.c_str());
		if (ImGui::InputTextWithHint("##Search", "Search...", buffer, sizeof(buffer)))
		{
			m_SearchFilter = buffer;
		}

		ImGui::SameLine();

		// TrashButton
		// 左クリック: Trashフォルダへ移動
		// 右クリック: 空にするメニュー表示
		if (ImGui::Button(ICON_FA_TRASH))
		{
			std::filesystem::path trashPath = m_BaseDirectory.parent_path() / ".Trash";
			if (std::filesystem::exists(trashPath))
			{
				ChangeDirectory(trashPath);
			}
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to Open Trash\nRight-Click to Empty");

		if (ImGui::BeginPopupContextItem("TrashCtx", ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Empty Trash"))
			{
				ImGui::CloseCurrentPopup();
				ImGui::OpenPopup("EmptyTrashPopup");

				std::filesystem::path trashPath = m_BaseDirectory.parent_path() / ".Trash";
				if (std::filesystem::exists(trashPath))
				{
					for (const auto& entry : std::filesystem::directory_iterator(trashPath))
						std::filesystem::remove_all(entry.path());
				}
			}
			ImGui::EndPopup();
		}
	}

	void ProjectBrowserPanel::DrawDirectoryTree()
	{
		DrawTreeNode(m_BaseDirectory);

		// Trashもツリーに表示する場合
		std::filesystem::path trashPath = m_BaseDirectory.parent_path() / ".Trash";
		if (std::filesystem::exists(trashPath))
		{
			DrawTreeNode(trashPath);
		}
	}

	void ProjectBrowserPanel::DrawTreeNode(const std::filesystem::path& path)
	{
		std::string filename = path.filename().string();
		if (filename.empty()) filename = path.string();
		if (filename == ".Trash") filename = "Trash";

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

		// リスト化
		std::vector<std::filesystem::directory_entry> entries;
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				entries.push_back(entry);
			}
		}
		catch (...) {}

		// 描画ループ
		for (const auto& entry : entries)
		{
			if (!MatchesFilter(entry.path())) continue;

			DrawEntryItem(entry);
			ImGui::NextColumn();
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

		// Drag & Drop
		if (isDir)
		{
			HandleDragDropTarget(path);
		}

		// Drag & Drop
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			const wchar_t* itemPath = path.c_str();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
			ImGui::Text("%s", filename.c_str());
			ImGui::EndDragDropSource();
		}

		// Interaction
		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (isDir)
				{
					ChangeDirectory(path);
				}
				else
				{
					std::filesystem::path preferredPath = path;
					preferredPath.make_preferred();
					EditorFileSystem::OpenExternal(preferredPath);
				}
			}
			else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				bool ctrl = ImGui::GetIO().KeyCtrl;
				SelectItem(path, ctrl);
			}
		}

		// Pop Style
		if (isSelected)
		{
			ImGui::PopStyleColor();
		}
		ImGui::PopStyleColor();

		// Context Menu (Right Click)
		if (ImGui::BeginPopupContextItem("ItemCtx"))
		{
			if (m_SelectedItems.find(path) == m_SelectedItems.end()) SelectItem(path, false);

			// Show in Explorer
			if (ImGui::MenuItem(ICON_FA_EYE " Show in Explorer"))
			{
				std::filesystem::path p = path.parent_path();
				p.make_preferred();
				EditorFileSystem::OpenInExplorer(p);
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Rename", "F2"))
			{
				m_IsRenaming = true;
				m_RenameFocus = true;
				m_RenamingPath = path;
				strncpy_s(m_RenameBuffer, filename.c_str(), sizeof(m_RenameBuffer));
			}
			if (ImGui::MenuItem("Delete", "Del")) m_ShowDeleteDialog = true;

			ImGui::EndPopup();
		}

		// Rename Input
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
		// Background Context Menu
		if (ImGui::BeginPopupContextWindow("ProjectBrowserContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem(ICON_FA_EYE " Show in Explorer"))
			{
				std::filesystem::path p = m_CurrentDirectory;
				p.make_preferred();
				EditorFileSystem::OpenInExplorer(p);
			}
			ImGui::Separator();

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
				if (ImGui::MenuItem("Material", ".mat"))
				{
					std::string content = "{\n\t\"Shader\": \"Basic.hlsl\",\n\t\"Albedo\": [1.0, 1.0, 1.0]\n}";
					CreateFileFromTemplate("NewMaterial.mat", content);
				}
				if (ImGui::MenuItem("Scene", ".span"))
				{
					std::string content = "{\n\t\"Entities\": []\n}";
					CreateFileFromTemplate("NewScene.span", content);
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
				ImGui::EndMenu();
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
				if (MatchesFilter(entry.path()))
				{
					m_SelectedItems.insert(entry.path());
				}
			}
		}
	}

	void ProjectBrowserPanel::SelectItem(const std::filesystem::path& path, bool multiSelect)
	{
		if (multiSelect)	// Ctrl + Click
		{
			if (m_SelectedItems.find(path) != m_SelectedItems.end())
			{
				m_SelectedItems.erase(path);
				SelectionManager::DeselectAsset(path);	// 同期: 選択解除
			}
			else
			{
				m_SelectedItems.insert(path);
				m_LastSelectedPath = path;
				SelectionManager::AddAsset(path);		// 同期: 追加選択
			}
		}
		else // Normal Click
		{
			// Shiftキーの判定
			bool shift = ImGui::GetIO().KeyShift;

			if (shift && !m_LastSelectedPath.empty() && m_LastSelectedPath != path)
			{
				// 選択範囲ロジック
				m_SelectedItems.clear();

				// 現在の全ディレクトリ内のアイテムを走査後、StartとEndの間にあるものを追加
				bool inRange = false;
				std::filesystem::path start = m_LastSelectedPath;
				std::filesystem::path end = path;

				std::vector<std::filesystem::path> currentItems;
				for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
				{
					if (entry.path().filename() != ".Trash" && entry.path().extension() != ".meta")
					{
						// フィルタ使用中ならフィルタも考慮
						if (!m_SearchFilter.empty() && entry.path().filename().string().find(m_SearchFilter) == std::string::npos) continue;

						currentItems.push_back(entry.path());
					}
				}

				// 範囲選択の実行
				SelectionManager::Clear();
				bool selecting = false;

				// インデックスベースで再取得
				int startIdx = -1, endIdx = -1;
				for (int i = 0; i < currentItems.size(); ++i)
				{
					if (currentItems[i] == start) startIdx = i;
					if (currentItems[i] == end) endIdx = i;
				}

				if (startIdx != -1 && endIdx != -1)
				{
					int minIdx = std::min(startIdx, endIdx);
					int maxIdx = std::max(startIdx, endIdx);
					for (int i = minIdx; i <= maxIdx; ++i)
					{
						m_SelectedItems.insert(currentItems[i]);
						SelectionManager::AddAsset(currentItems[i]);
					}
				}
			}
			else
			{
				// 通常の単一選択
				m_SelectedItems.clear();
				m_SelectedItems.insert(path);
				m_LastSelectedPath = path;
				SelectionManager::SelectAsset(path);
			}
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

		if (m_DirectoryWatcher) m_DirectoryWatcher->SetDirectory(m_CurrentDirectory);
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
		if (!m_UndoStack.empty())
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
