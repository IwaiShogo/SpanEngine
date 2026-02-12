#include <SpanEngine.h>
#include <SpanEditor.h>
#include "ProjectBrowserPanel.h"
#include <imgui.h>

// FontAwesomeマクロが無い場合のフォールバック定義
#ifndef ICON_FA_FOLDER
#define ICON_FA_FOLDER "[D]"
#endif
#ifndef ICON_FA_FILE
#define ICON_FA_FILE "[F]"
#endif
#ifndef ICON_FA_ARROW_LEFT
#define ICON_FA_ARROW_LEFT "<-"
#endif
#ifndef ICON_FA_MAGNIFYING_GLASS
#define ICON_FA_MAGNIFYING_GLASS "Search"
#endif
#ifndef ICON_FA_GEAR
#define ICON_FA_GEAR "Settings"
#endif

namespace Span
{
	static AutoRegisterPanel<ProjectBrowserPanel> _reg("Project");

	ProjectBrowserPanel::ProjectBrowserPanel()
		: EditorPanel("Project")
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

		// 全体のレイアウト設定
		DrawNavBar();
		ImGui::Separator();

		// 2カラムレイアウト (左: ツリー、右: コンテンツ)
		static float leftPaneWidth = 200.0f;
		ImGui::Columns(2, "ProjectBrowserColumns", true);

		// 初回のみ幅を設定
		if (ImGui::GetFrameCount() == 1)
		{
			ImGui::SetColumnWidth(0, leftPaneWidth);
		}

		// --- Left Pane: Directory Tree ---
		ImGui::BeginChild("DirectoryTree", ImVec2(0, 0), false);
		DrawDirectoryTree();
		ImGui::EndChild();

		ImGui::NextColumn();

		// --- Right Pane: Content Area ---
		ImGui::BeginChild("ContentArea", ImVec2(0, 0), false);
		DrawContentArea();
		ImGui::EndChild();

		ImGui::Columns(1);

		ImGui::End();
	}

	void ProjectBrowserPanel::DrawNavBar()
	{
		// 戻るボタン
		if (ImGui::Button(ICON_FA_ARROW_LEFT))
		{
			if (m_CurrentDirectory != m_BaseDirectory)
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}
		ImGui::SameLine();

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
			if (ImGui::Button(name.c_str()))
			{
				m_CurrentDirectory = *it;
			}
			if (std::next(it) != parts.rend())
			{
				ImGui::SameLine();
				ImGui::Text(">");
				ImGui::SameLine();
			}
		}

		ImGui::SameLine();

		// 右寄せグループ
		float settingsWidth = 150.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - settingsWidth);

		// 検索バー
		ImGui::PushItemWidth(100);
		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		std::strncpy(buffer, m_SearchFilter.c_str(), sizeof(buffer));
		if (ImGui::InputTextWithHint("##Search", ICON_FA_MAGNIFYING_GLASS " Filter...", buffer, sizeof(buffer)))
		{
			m_SearchFilter = std::string(buffer);
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();

		// 設定ボタン
		if (ImGui::Button(ICON_FA_GEAR))
		{
			ImGui::OpenPopup("ProjectBrowserSettings");
		}

		if (ImGui::BeginPopup("ProjectBrowserSettings"))
		{
			ImGui::Text("View Settings");
			ImGui::Separator();

			ImGui::SliderFloat("Icon Size", &m_ThumbnailSize, 32.0f, 256.0f);
			ImGui::SliderFloat("Padding", &m_Padding, 0.0f, 32.0f);
			ImGui::Checkbox("Show Hidden Files", &m_ShowHiddenFiles);

			ImGui::EndPopup();
		}
	}

	void ProjectBrowserPanel::DrawDirectoryTree()
	{
		DrawDirectoryNode(m_BaseDirectory);
	}

	void ProjectBrowserPanel::DrawDirectoryNode(const std::filesystem::path& path)
	{
		// ディレクトリ名を取得
		std::string filename = path.filename().string();
		if (filename.empty()) filename = path.string();

		// ツリーノードのフラグ
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiSelectableFlags_SpanAvailWidth;

		// カレントディレクトリなら選択状態にする
		if (path == m_CurrentDirectory)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		// サブディレクトリがあるか確認 (Leaf判定)
		bool isLeaf = true;
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(path))
			{
				if (entry.is_directory())
				{
					isLeaf = false;
					break;
				}
			}
		}
		catch (...) {}

		if (isLeaf)
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		// ノード描画
		bool opened = ImGui::TreeNodeEx(path.string().c_str(), flags, "%s %s", ICON_FA_FOLDER, filename.c_str());

		// クリックでディレクトリ移動
		if (ImGui::IsItemClicked())
		{
			m_CurrentDirectory = path;
		}

		// 再帰描画
		if (opened)
		{
			if (!isLeaf)
			{
				try
				{
					for (const auto& entry : std::filesystem::directory_iterator(path))
					{
						if (entry.is_directory())
						{
							// 隠しフォルダスキップ
							if (!m_ShowHiddenFiles && entry.path().filename().string()[0] == '.') continue;
							DrawDirectoryNode(entry.path());
						}
					}
				}
				catch (...) {}
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

		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				const auto& path = entry.path();
				std::string filename = path.filename().string();

				// 隠しファイルスキップ
				if (!m_ShowHiddenFiles && filename[0] == '.') continue;

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
	}

	void ProjectBrowserPanel::DrawEntryItem(const std::filesystem::directory_entry& entry)
	{
		std::string filename = entry.path().filename().string();
		bool isDirectory = entry.is_directory();

		// アイコン
		// TODO: 将来的には拡張子に応じたテクスチャIDを取得して ImGui::ImageButton を使用する
		const char* icon = isDirectory ? ICON_FA_FOLDER : ICON_FA_FILE;

		ImGui::PushID(filename.c_str());

		ImVec2 size(m_ThumbnailSize, m_ThumbnailSize);

		// スタイルの調整
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		if (ImGui::Button(icon, size))
		{
			if (isDirectory)
			{
				m_CurrentDirectory = entry.path();
			}
		}

		// ドラッグ&ドロップの実装
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			// ペイロードとしてファイルの絶対パスを渡す
			std::wstring pathStr = entry.path().wstring();
			ImGui::SetDragDropPayload("PROJECT_BROWSER_ITEM", pathStr.c_str(), (pathStr.length() + 1) * sizeof(wchar_t));

			// ドラッグ中のプレビュー
			ImGui::Text("%s %s", icon, filename.c_str());
			ImGui::EndDragDropSource();
		}

		ImGui::PopStyleColor();

		// テキスト描画
		ImGui::TextWrapped("%s", filename.c_str());

		ImGui::PopID();
	}
}
