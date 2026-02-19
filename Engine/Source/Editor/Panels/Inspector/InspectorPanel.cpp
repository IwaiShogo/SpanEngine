#include "InspectorPanel.h"

#include <SpanEngine.h>
#include <SpanEditor.h>

#include "Editor/Utils/EditorFileSystem.h"
#include "Runtime/Core/TagManager.h"

#include <imgui.h>
#include <imgui_internal.h>

// Fallback Icons
#ifndef ICON_FA_FILE
#define ICON_FA_FILE "[FILE]"
#endif
#ifndef ICON_FA_FILES_O
#define ICON_FA_FILES_O "[FILES]"
#endif
#ifndef ICON_FA_IMAGE
#define ICON_FA_IMAGE "[IMG]"
#endif
#ifndef ICON_FA_CUBE
#define ICON_FA_CUBE "[MESH]"
#endif

namespace Span
{
	static AutoRegisterPanel<InspectorPanel> _reg("Inspector");

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Inspector", &isOpen);

		// 1. 選択タイプの取得
		SelectionType type = SelectionManager::GetType();

		if (type == SelectionType::None)
		{
			// 何も選択されていない場合
			ImGui::TextDisabled("No Selection");
		}
		else if (type == SelectionType::Asset)
		{
			// アセットインスペクターの描画
			const auto& assets = SelectionManager::GetAssetSelections();
			DrawAssetInspector(assets);
		}
		else if (type == SelectionType::Entity)
		{
			// 1. 選択ターゲットの取得
			Entity selected = SelectionManager::GetPrimaryEntity();
			World& world = Application::Get().GetWorld();

			// 未選択または無効なEntityの場合はここで終了
			if (selected.IsNull() || !world.IsAlive(selected))
			{
				ImGui::Text("No Entity Selected");
				ImGui::End();
				return;
			}

			// 2. ヘッダー情報
			ImGui::TextDisabled("ID: %d | Gen: %d", selected.ID.Index, selected.ID.Generation);
			ImGui::Separator();
			ImGui::Spacing();

			// 3. 基本情報 (Name, Active, Tag)
			bool isActive = true;
			if (Active* a = world.GetComponentPtr<Active>(selected))
			{
				isActive = a->IsActive;
			}
			else
			{
				world.AddComponent<Active>(selected);
			}

			if (ImGui::Checkbox("##Active", &isActive))
			{
				if (Active* a = world.GetComponentPtr<Active>(selected)) a->IsActive = isActive;
			}

			ImGui::SameLine();

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
			if (world.HasComponent<Name>(selected))
			{
				// 名前コンポーネントの参照を取得
				auto& nameComponent = world.GetComponent<Name>(selected);

				ImGui::PushItemWidth(-1);
				ImGui::InputText("##Name", nameComponent.Value.data(), nameComponent.Value.Capacity());
				ImGui::PopItemWidth();
			}
			else
			{
				// Nameコンポーネントが無い場合は追加ボタンか、ダミー表示
				char buf[64];
				sprintf_s(buf, "Entity %d", selected.ID.Index);
				if (ImGui::InputText("##Name", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					world.AddComponent<Name>(selected);
					Name& n = world.GetComponent<Name>(selected);
					n.Value = buf;
				}
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();
			ImGui::TextDisabled("Static");

			ImGui::Spacing();
			ImGui::Separator();

			{
				float labelWidth = 50.0f;

				// --- Tag ---
				ImGui::Text("Tag"); ImGui::SameLine(labelWidth);

				// [+] ボタンの幅を考慮してComboの幅を設定
				float addBtnWidth = 30.0f;
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f - 10 - addBtnWidth);

				// TagManagerからタグリストを取得
				const auto& registeredTags = TagManager::Get().GetAllTags();

				// 現在のエンティティが持っているタグ名を取得
				std::string currentTagStr = "Untagged";
				if (Tag* t = world.GetComponentPtr<Tag>(selected))
				{
					currentTagStr = t->Value.data();
				}
				else
				{
					world.AddComponent<Tag>(selected, Tag("Untagged"));
				}

				// 現在のタグがリストの何番目か
				int currentTagIndex = 0;
				for (int i = 0; i < registeredTags.size(); i++)
				{
					if (registeredTags[i] == currentTagStr)
					{
						currentTagIndex = i;
						break;
					}
				}

				// ImGui::Combo用のC文字列配列を作成
				std::vector<const char*> tagCStrs;
				tagCStrs.reserve(registeredTags.size());
				for (const auto& tag : registeredTags) tagCStrs.push_back(tag.c_str());

				if (ImGui::Combo("##Tag", &currentTagIndex, tagCStrs.data(), (int)tagCStrs.size()))
				{
					// 変更をコンポーネントに適用
					if (Tag* t = world.GetComponentPtr<Tag>(selected))
					{
						t->Value = registeredTags[currentTagIndex];
					}
				}

				// [+] タグ追加ボタン
				ImGui::SameLine();
				if (ImGui::Button("+##AddTag", ImVec2(addBtnWidth, 0)))
				{
					ImGui::OpenPopup("AddTagPopup");
				}

				// タグ追加用ポップアップ
				if (ImGui::BeginPopup("AddTagPopup"))
				{
					ImGui::TextDisabled("Add New Tag");
					ImGui::Separator();

					static char newTagBuffer[64] = "";
					if (ImGui::InputText("##NewTagName", newTagBuffer, sizeof(newTagBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
					{
						std::string newTag(newTagBuffer);
						if (TagManager::Get().AddTag(newTag))
						{
							// 追加後、即座にそのタグを設定する
							if (Tag* t = world.GetComponentPtr<Tag>(selected))
							{
								t->Value = newTag;
							}
						}
						newTagBuffer[0] = '\n';
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::SameLine();

				// --- Layer ---
				ImGui::Text("Layer"); ImGui::SameLine(labelWidth + ImGui::GetContentRegionAvail().x * 0.5f + 5);
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

				const char* layers[] = { "Default", "Transparent", "Ignore Raycast", "Water", "UI" };
				int currentLayerIndex = 0;

				if (Layer* l = world.GetComponentPtr<Layer>(selected))
				{
					currentLayerIndex = l->Value;
					if (currentLayerIndex < 0 || currentLayerIndex >= IM_ARRAYSIZE(layers)) currentLayerIndex = 0;
				}
				else
				{
					world.AddComponent<Layer>(selected, Layer(0));
				}

				if (ImGui::Combo("##Layer", &currentLayerIndex, layers, IM_ARRAYSIZE(layers)))
				{
					if (Layer* l = world.GetComponentPtr<Layer>(selected)) {
						l->Value = currentLayerIndex;
					}
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// 4. コンポーネントリストの描画 (Reflection)
			// 登録されている全コンポーネントメタデータを取得
			auto& components = ComponentRegistry::GetAll();

			// ソート (Transformを最優先、あとは名前順)
			std::sort(components.begin(), components.end(), [](const ComponentMetadata& a, const ComponentMetadata& b)
				{
					if (a.Name == "Transform") return true;
					if (b.Name == "Transform") return false;

					return a.Order < b.Order;
				});

			// 描画ループ
			for (size_t i = 0; i < components.size(); ++i)
			{
				auto& meta = components[i];

				bool isRemove = false;

				// 基本コンポーネントはリストに出さない
				if (meta.Name == "Name" || meta.Name == "Tag" || meta.Name == "Layer" || meta.Name == "Active") continue;

				ImGui::PushID(i);

				// 描画実行
				meta.DrawFunc(selected, world);

				ImGui::PopID();
			}

			// 5. コンポーネント追加ボタン (Add Component)
			ImGui::Spacing();
			ImGui::Separator();

			if (ImGui::Button("Add Component", ImVec2(-1, 0)))
			{
				ImGui::OpenPopup("AddComponentPopup");
			}

			// 追加ポップアップ
			if (ImGui::BeginPopup("AddComponentPopup"))
			{
				// コンポーネント名をソートして表示
				std::vector<const ComponentMetadata*> sortedMetas;
				for (const auto& meta : components) sortedMetas.push_back(&meta);
				std::sort(sortedMetas.begin(), sortedMetas.end(), [](const auto* a, const auto* b) { return a->Name < b->Name; });

				for (const auto* meta : sortedMetas)
				{
					if (meta->Name == "Name" || meta->Name == "Tag" || meta->Name == "Layer" || meta->Name == "Active") continue;

					if (meta->HasFunc && meta->HasFunc(selected, world)) continue;

					if (ImGui::MenuItem(meta->Name.c_str()))
					{
						if (meta->Name.c_str())
						{
							meta->AddFunc(selected, world);
						}
					}
				}
				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}

	void InspectorPanel::DrawAssetInspector(const std::vector<std::filesystem::path>& paths)
	{
		if (paths.empty()) return;

		// --- 複数選択時のヘッダー表示 ---
		if (paths.size() > 1)
		{
			ImGui::Text("%s %d items selected", ICON_FA_FILES_O, (int)paths.size());
			ImGui::Separator();

			// 選択リストの表示
			ImGui::Text("Selected Assets:");
			ImGui::BeginChild("MultiSelectAssets", ImVec2(0, 100), true);
			for (const auto& path : paths)
			{
				std::string name = path.filename().string();
				const char* icon = ICON_FA_FILE;
				// 簡易アイコン分岐
				std::string ext = path.extension().string();
				if (ext == ".png" || ext == ".jpg") icon = ICON_FA_IMAGE;
				else if (ext == ".span") icon = ICON_FA_CUBE;

				ImGui::Text("%s %s", icon, name.c_str());
			}
			ImGui::EndChild();

			ImGui::Separator();
			ImGui::TextDisabled("Primary Selection Preview:");
			ImGui::Spacing();
		}

		// --- Primary Assetの詳細表示 ---
		// 複数選択時は、リストの最後の要素をPrimaryとして扱う
		const std::filesystem::path& path = paths.back();
		std::string filename = path.filename().string();
		std::string ext = path.extension().string();

		// ヘッダー (ファイル名)
		ImGui::Text("%s  %s", ICON_FA_FILE, filename.c_str());
		if (paths.size() == 1)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(%s)", ext.c_str());
		}

		ImGui::Separator();

		// パス情報
		std::string pathStr = path.string();
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		strncpy_s(buffer, pathStr.c_str(), sizeof(buffer) - 1);
		ImGui::InputText("Path", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);

		ImGui::Spacing();

		// --- ファイルタイプ別表示 ---
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga")
		{
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Type: Texture");

			// 画像プレビュー
			if (void* texID = AssetManager::Get().GetEditorThumbnail(path))
			{
				float availWidth = ImGui::GetContentRegionAvail().x;
				float size = std::min(availWidth, 256.0f);
				ImGui::Image((ImTextureID)texID, ImVec2(size, size));
			}
		}
		else if (ext == ".fbx" || ext == ".obj" || ext == ".gltf")
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Type: 3D Model");
			// モデル情報の表示（頂点数などはModelLoader経由で取得が必要）
			ImGui::Text("Import Settings:");
			static bool importMesh = true;
			static bool importMaterials = true;
			ImGui::Checkbox("Import Mesh", &importMesh);
			ImGui::Checkbox("Import Materials", &importMaterials);

			if (ImGui::Button("Reimport"))
			{
				// TODO: Reimport logic
			}
		}
		else if (ext == ".mat")
		{
			ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Type: Material");
			ImGui::Spacing();

			if (ImGui::Button("Open in Material Editor", ImVec2(-1, 0)))
			{
				// TODO: Open Material Editor Window
			}

			// 簡易プロパティ編集 (JSONパースが必要)
			// 本格実装ではAssetManager経由でMaterialインスタンスを取得して描画する
		}
		else if (ext == ".cpp" || ext == ".h")
		{
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Type: C++ Source");
			if (ImGui::Button("Open in IDE", ImVec2(-1, 0)))
			{
				EditorFileSystem::OpenExternal(path);
			}
		}
		else
		{
			ImGui::TextDisabled("Unknown Asset Type");
		}

		// 共通フッター
		ImGui::Spacing();
		ImGui::Separator();
		if (std::filesystem::exists(path.string() + ".meta"))
		{
			ImGui::TextDisabled("GUID: [Cached in .meta]");
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: No .meta file");
		}
	}
}
