#include "InspectorPanel.h"

#include <SpanEngine.h>
#include <SpanEditor.h>

#include "Editor/Utils/EditorFileSystem.h"
#include "Editor/Commands/FileCommands.h"
#include "Runtime/Core/TagManager.h"
#include "Runtime/Core/LayerManager.h"

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
#ifndef ICON_FA_SEARCH
#define ICON_FA_SEARCH "[?]"
#endif
#ifndef ICON_FA_TAGS
#define ICON_FA_TAGS "[TAGS]"
#endif
#ifndef ICON_FA_TRASH
#define ICON_FA_TRASH "[DEL]"
#endif
#ifndef ICON_FA_PLUS
#define ICON_FA_PLUS "[+]"
#endif
#ifndef ICON_FA_LOCK
#define ICON_FA_LOCK "[L]"
#endif
#ifndef ICON_FA_UNLOCK
#define ICON_FA_UNLOCK "[U]"
#endif

namespace Span
{
	static AutoRegisterPanel<InspectorPanel> _reg("Inspector");

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Inspector", &isOpen);

		// ロック機能のトグルボタン
		float lockBtnWidth = 30.0f;
		ImGui::SameLine(ImGui::GetWindowWidth() - lockBtnWidth - ImGui::GetStyle().WindowPadding.x);

		if (m_IsLocked)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
			if (ImGui::Button(ICON_FA_LOCK, ImVec2(lockBtnWidth, 0))) m_IsLocked = false;
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Unlock Inspector");
		}
		else
		{
			if (ImGui::Button(ICON_FA_UNLOCK, ImVec2(lockBtnWidth, 0)))
			{
				m_IsLocked = true;
				m_LockedType = SelectionManager::GetType();
				if (m_LockedType == SelectionType::Asset) m_LockedAssets = SelectionManager::GetAssetSelections();
				if (m_LockedType == SelectionType::Entity) m_LockedEntity = SelectionManager::GetPrimaryEntity();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lock Inspector");
		}

		// 1. 選択タイプの取得
		SelectionType type = m_IsLocked ? m_LockedType : SelectionManager::GetType();

		if (type == SelectionType::None)
		{
			// 何も選択されていない場合
			ImGui::TextDisabled("No Selection");
		}
		else if (type == SelectionType::Asset)
		{
			// アセットインスペクターの描画
			const auto& assets = m_IsLocked ? m_LockedAssets : SelectionManager::GetAssetSelections();
			DrawAssetInspector(assets);
		}
		else if (type == SelectionType::Entity)
		{
			// 1. 選択ターゲットの取得
			Entity selected = m_IsLocked ? m_LockedEntity : SelectionManager::GetPrimaryEntity();
			World& world = Application::Get().GetWorld();

			if (selected.IsNull() || !world.IsAlive(selected))
			{
				ImGui::TextDisabled("Invalid Entity");
				m_IsLocked = false;
			}
			else
			{
				DrawEntityInspector(selected, world);
			}
		}

		// ポップアップの描画
		DrawTagEditorModal();
		DrawLayerEditorModal();

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

			DrawMaterialEditor(path);
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

	void InspectorPanel::DrawEntityInspector(Entity selected, World& world)
	{
		// 1. ヘッダー情報
		ImGui::TextDisabled("ID: %d | Gen: %d", selected.ID.Index, selected.ID.Generation);
		ImGui::Separator();
		ImGui::Spacing();

		// 2. Active & Name
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
		ImGui::PopItemWidth();

		// TODO: Staticを将来的に実装する。
		ImGui::SameLine();
		ImGui::TextDisabled("Static");
		ImGui::Spacing();
		ImGui::Separator();

		// 3. Tag & Layer
		float labelWidth = 50.0f;

		// --- TAG ---
		ImGui::Text("Tag");
		ImGui::SameLine(labelWidth);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f - 10);

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

		if (ImGui::BeginCombo("##Tag", currentTagStr.c_str()))
		{
			// 検索バー
			static char searchBuf[64] = "";
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##TagSearch", ICON_FA_SEARCH " Search...", searchBuf, sizeof(searchBuf));

			std::string searchQ = searchBuf;
			std::transform(searchQ.begin(), searchQ.end(), searchQ.begin(), [](unsigned char c) { return std::tolower(c); });
			ImGui::Separator();

			// タグのリストアップとフィルタリング
			for (const auto& tag : TagManager::Get().GetAllTags())
			{
				std::string lowerTag = tag;
				std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), [](unsigned char c) { return std::tolower(c); });

				// 検索文字列が含まれない場合はスキップ
				if (!searchQ.empty() && lowerTag.find(searchQ) == std::string::npos) continue;

				bool isSelected = (currentTagStr == tag);
				if (ImGui::Selectable(tag.c_str(), isSelected))
				{
					if (Tag* t = world.GetComponentPtr<Tag>(selected)) t->Value = tag;
					memset(searchBuf, 0, sizeof(searchBuf));
				}

				if (isSelected && ImGui::IsWindowAppearing())
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::Separator();

			// Tag Editorへのアクセスボタン
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
			if (ImGui::Selectable(ICON_FA_TAGS " Edit Tags..."))
			{
				m_OpenTagEditor = true;
				memset(searchBuf, 0, sizeof(searchBuf));
			}
			ImGui::PopStyleColor();

			ImGui::EndCombo();
		}

		ImGui::SameLine();

		// --- LAYER ---
		ImGui::Text("Layer");
		ImGui::SameLine(labelWidth + ImGui::GetContentRegionAvail().x * 0.5f + 5);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

		uint8_t currentLayerIndex = 0;
		if (Layer* l = world.GetComponentPtr<Layer>(selected)) currentLayerIndex = l->Value;
		else world.AddComponent<Layer>(selected, Layer{ 0 });	// Default

		// 有効なレイヤー名だけを取得
		std::string currentLayerName = LayerManager::Get().GetLayerName(currentLayerIndex);
		if (currentLayerName.empty()) currentLayerName = "Unknown Layer";

		if (ImGui::BeginCombo("##Layer", currentLayerName.c_str()))
		{
			for (int i = 0; i < 32; i++)
			{
				if (!LayerManager::Get().IsValidLayer(i)) continue;	// 名前の無いレイヤーは非表示

				bool isSelected = (currentLayerIndex == i);
				std::string displayName = std::to_string(i) + ": " + LayerManager::Get().GetLayerName(i);

				if (ImGui::Selectable(displayName.c_str(), isSelected))
				{
					if (Layer* l = world.GetComponentPtr<Layer>(selected)) l->Value = i;
				}
				if (isSelected && ImGui::IsWindowAppearing()) ImGui::SetItemDefaultFocus();
			}

			ImGui::Separator();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
			if (ImGui::Selectable(ICON_FA_FILES_O "Edit Layers..."))
			{
				m_OpenLayerEditor = true;
			}
			ImGui::PopStyleColor();

			ImGui::EndCombo();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 4. コンポーネントリストの描画
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

			// 基本コンポーネントはリストに出さない
			if (meta.Name == "Name" || meta.Name == "Tag" || meta.Name == "Layer" || meta.Name == "Active" || meta.Name == "LocalToWorld" || meta.Name == "Relationship" || meta.Name == "IDComponent") continue;

			ImGui::PushID(i);

			// 描画実行
			meta.DrawFunc(selected, world);

			ImGui::PopID();
		}

		// 5. AddComponent
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

	void InspectorPanel::DrawMaterialEditor(const std::filesystem::path& path)
	{
		ImGui::Separator();
		ImGui::Spacing();

		// AssetManagerからマテリアルを取得
		auto editMaterial = AssetManager::Get().GetMaterial(path.string());
		if (!editMaterial)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to load Material");
			return;
		}

		bool isChanged = false;

		// --- ヘッダー ---
		std::string fileNameOnly = path.stem().string();
		char nameBuf[256];
		strncpy_s(nameBuf, fileNameOnly.c_str(), sizeof(nameBuf) - 1);

		if (ImGui::InputText("Material Name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			std::string newName = nameBuf;
			if (!newName.empty() && newName != fileNameOnly)
			{
				std::filesystem::path newPath = path.parent_path() / (newName + ".mat");

				// 時差氏のリネーム処理
				if (!std::filesystem::exists(newPath))
				{
					std::filesystem::rename(path, newPath);
					SelectionManager::Clear();
					SelectionManager::SelectAsset(newPath);
					SPAN_LOG("Material renamed to: %s", newName.c_str());
					return;
				}
				else
				{
					SPAN_ERROR("A file with that name already exists!");
				}
			}
		}

		ImGui::Spacing();

		// プレビューのレンダリングと表示
		Renderer& renderer = Application::Get().GetRenderer();
		m_MaterialPreviewer.Initialize(renderer.GetDevice());
		m_MaterialPreviewer.Render(renderer.GetCommandList(), &renderer, editMaterial.get());

		// ImGui 上にプレビューテクスチャを中央ぞろえで描画
		if (void* texID = m_MaterialPreviewer.GetTextureID())
		{
			float windowWidth = ImGui::GetWindowSize().x;
			float imageSize = 256.0f;
			ImGui::SetCursorPosX((windowWidth - imageSize) * 0.5f);

			// 角を少し丸くする
			ImGui::Image((ImTextureID)texID, ImVec2(imageSize, imageSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
			ImGui::Spacing();
		}

		ImGui::SeparatorText("Render Settings");

		// --- レンダーステート ---
		int blendMode = static_cast<int>(editMaterial->GetBlendMode());
		if (ImGui::Combo("Blend Mode", &blendMode, "Opaque\0Transparent\0Cutout\0"))
		{
			editMaterial->SetBlendMode(static_cast<BlendMode>(blendMode));
			isChanged = true;
		}

		int cullMode = static_cast<int>(editMaterial->GetCullMode());
		if (ImGui::Combo("Cull Mode", &cullMode, "Back\0Front\0None\0"))
		{
			editMaterial->SetCullMode(static_cast<CullMode>(cullMode));
			isChanged = true;
		}

		ImGui::Spacing();
		ImGui::SeparatorText("PBR Properties & Texture");

		// --- PBR パラメータ & テクスチャスロット---
		MaterialData& data = editMaterial->GetData();

		auto DrawPBRSlot = [&](const char* label, float* colorOrValue, Texture* currentTex, uint64_t& texGUID, std::function<void(Texture*)> setter, int colorComps)
		{
			ImGui::PushID(label);

			// 1. サムネイルボタンの描画
			ImVec2 texSize(48.0f, 48.0f);
			void* texID = currentTex ? currentTex->GetImGuiTextureID() : nullptr;

			if (texID)
			{
				if (ImGui::ImageButton("##texBtn", (ImTextureID)texID, texSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1))) {}
			}
			else
			{
				ImGui::Button("None", texSize);
			}

			// ドラッグ＆ドロップの受け入れ
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* pathStr = (const wchar_t*)payload->Data;
					std::filesystem::path droppedPath(pathStr);
					std::string ext = droppedPath.extension().string();

					if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga")
					{
						auto tex = AssetManager::Get().GetTexture(droppedPath.string());
						if (tex)
						{
							setter(tex.get());
							texGUID = tex->Handle;
							isChanged = true;
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			// 右クリックでテクスチャのクリア
			if (ImGui::BeginPopupContextItem("TexClearMenu"))
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					setter(nullptr);
					texGUID = 0;
					isChanged = true;
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();

			// 2. ラベルとプロパティの描画
			ImGui::BeginGroup();
			ImGui::Text("%s", label);

			if (colorOrValue != nullptr)
			{
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (colorComps == 4)
				{
					if (ImGui::ColorEdit4("##Color", colorOrValue, ImGuiColorEditFlags_NoInputs)) isChanged = true;
				}
				else if (colorComps == 3) // Emissive用 (Vector3)
				{
					if (ImGui::ColorEdit3("##Color", colorOrValue, ImGuiColorEditFlags_NoInputs)) isChanged = true;
				}
				else
				{
					if (ImGui::SliderFloat("##Value", colorOrValue, 0.0f, 1.0f)) isChanged = true;
				}
			}
			ImGui::EndGroup();

			ImGui::PopID();
			ImGui::Spacing();
		};

		DrawPBRSlot("Albedo", &data.AlbedoColor.x, editMaterial->GetAlbedoMap(), editMaterial->AlbedoMapGUID, [&](Texture* t) { editMaterial->SetAlbedoMap(t); }, 4);
		DrawPBRSlot("Metallic", &data.Metallic, editMaterial->GetMetallicMap(), editMaterial->MetallicMapGUID, [&](Texture* t) { editMaterial->SetMetallicMap(t); }, 0);
		DrawPBRSlot("Roughness", &data.Roughness, editMaterial->GetRoughnessMap(), editMaterial->RoughnessMapGUID, [&](Texture* t) { editMaterial->SetRoughnessMap(t); }, 0);
		DrawPBRSlot("Normal", nullptr, editMaterial->GetNormalMap(), editMaterial->NormalMapGUID, [&](Texture* t) { editMaterial->SetNormalMap(t); }, 0);
		DrawPBRSlot("AO", &data.AO, editMaterial->GetAOMap(), editMaterial->AOMapGUID, [&](Texture* t) { editMaterial->SetAOMap(t); }, 0);
		DrawPBRSlot("Emissive", &data.EmissiveColor.x, editMaterial->GetEmissiveMap(), editMaterial->EmissiveMapGUID, [&](Texture* t) { editMaterial->SetEmissiveMap(t); }, 3);

		if (editMaterial->GetBlendMode() == BlendMode::Cutout)
		{
			if (ImGui::SliderFloat("Alpha Cutoff", &data.Cutoff, 0.0f, 1.0f)) isChanged = true;
		}

		// ガラス用パラメータ
		if (ImGui::SliderFloat("Transmission", &data.Transmission, 0.0f, 1.0f)) isChanged = true;
		if (ImGui::SliderFloat("IOR (Index of Refraction)", &data.IOR, 1.0f, 3.0f)) isChanged = true;

		ImGui::Spacing();
		ImGui::SeparatorText("UV Settings");

		float tiling[2] = { data.Tiling.x, data.Tiling.y };
		if (ImGui::DragFloat2("Tiling", tiling, 0.01f))
		{
			data.Tiling = { tiling[0], tiling[1] };
			isChanged = true;
		}

		float offset[2] = { data.Offset.x, data.Offset.y };
		if (ImGui::DragFloat2("Offset", offset, 0.01f))
		{
			data.Offset = { offset[0], offset[1] };
			isChanged = true;
		}

		// 値が変更されたら即座に .mat ファイルに保存
		if (isChanged)
		{
			editMaterial->Update();
			editMaterial->Serialize(path);
		}
	}

	void InspectorPanel::DrawTagEditorModal()
	{
		if (m_OpenTagEditor)
		{
			ImGui::OpenPopup("TagEditorModal");
			m_OpenTagEditor = false;
		}

		bool isOpen = true;
		if (ImGui::BeginPopupModal("TagEditorModal", &isOpen, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextDisabled("Manage Global Project Tags");
			ImGui::Separator();
			ImGui::Spacing();

			// タグリストエリア (スクロール可能)
			ImGui::BeginChild("TagList", ImVec2(300, 200), true);
			auto allTags = TagManager::Get().GetAllTags();
			for (const auto& tag : allTags)
			{
				bool isProtected = TagManager::Get().IsProtectedTag(tag);
				ImGui::PushID(tag.c_str());

				ImGui::AlignTextToFramePadding();
				ImGui::Text(isProtected ? ICON_FA_TAGS " %s" : "   %s", tag.c_str());

				// 保護されていないタグのみ削除ボタンを表示
				if (!isProtected)
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 50.0f);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
					if (ImGui::Button(ICON_FA_TRASH))
					{
						TagManager::Get().RemoveTag(tag);
						ImGui::PopStyleColor();
						ImGui::PopID();
						break;
					}
					ImGui::PopStyleColor();
				}
				ImGui::PopID();
			}
			ImGui::EndChild();

			ImGui::Spacing();

			// 新規タグ追加エリア
			static char newTagBuf[64] = "";
			ImGui::SetNextItemWidth(200);
			bool enterPressed = ImGui::InputTextWithHint("##NewTag", "New Tag Name", newTagBuf, sizeof(newTagBuf), ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();

			bool isValid = TagManager::Get().IsValidTagName(newTagBuf);
			if (!isValid && strlen(newTagBuf) > 0)
			{
				ImGui::BeginDisabled();
			}

			if (ImGui::Button(ICON_FA_PLUS " Add", ImVec2(-1, 0)) || (enterPressed && isValid))
			{
				if (TagManager::Get().AddTag(newTagBuf))
				{
					memset(newTagBuf, 0, sizeof(newTagBuf));
				}
			}

			if (!isValid && strlen(newTagBuf) > 0)
			{
				ImGui::EndDisabled();
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: Alphanumeric and '_' only.");
			}

			ImGui::Spacing();
			ImGui::Separator();

			if (ImGui::Button("Close", ImVec2(-1, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void InspectorPanel::DrawLayerEditorModal()
	{
		if (m_OpenLayerEditor)
		{
			ImGui::OpenPopup("LayerEditorModal");
			m_OpenLayerEditor = false;
		}

		bool isOpen = true;
		ImGui::SetNextWindowSizeConstraints(ImVec2(650, 500), ImVec2(FLT_MAX, FLT_MAX));

		if (ImGui::BeginPopupModal("LayerEditorModal", &isOpen, ImGuiWindowFlags_NoSavedSettings))
		{
			// ヘッダーUI
			ImGui::TextDisabled("Tags & Layers Settings");
			ImGui::SameLine(ImGui::GetWindowWidth() - 250);

			// プリセット・メニュー
			if (ImGui::Button("Presets...")) ImGui::OpenPopup("MatrixPresets");
			if (ImGui::BeginPopup("MatrixPresets"))
			{
				ImGui::TextDisabled("Quick Presets");
				ImGui::Separator();
				if (ImGui::MenuItem("Enable All (Default)"))
				{
					for (int i = 0; i < 32; i++) for (int j = 0; j < 32; j++) LayerManager::Get().SetCollision(i, j, true);
				}
				if (ImGui::MenuItem("Disable All"))
				{
					for (int i = 0; i < 32; i++) for (int j = 0; j < 32; j++) LayerManager::Get().SetCollision(i, j, false);
				}
				if (ImGui::MenuItem("UI Only Isolation"))
				{
					for (int i = 0; i < 32; i++) for (int j = 0; j < 32; j++) LayerManager::Get().SetCollision(i, j, false);
					LayerManager::Get().SetCollision(5, 5, true);
				}
				ImGui::EndPopup();
			}

			if (ImGui::Button(ICON_FA_TRASH " Reset Matrix"))
			{
				for (int i = 0; i < 32; i++) for (int j = 0; j < 32; j++) LayerManager::Get().SetCollision(i, j, true);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset all collisions to Default (All Enabled)");

			ImGui::Separator();

			if (ImGui::BeginTabBar("SettingsTabs"))
			{
				// レイヤー名の編集
				// ==========================================================
				if (ImGui::BeginTabItem("Layers"))
				{
					ImGui::TextDisabled("Define user layers (8-31). System layers (0-7) are read-only.");
					ImGui::Spacing();

					ImGui::BeginChild("LayerNamesRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), true);
					for (int i = 0; i < 32; ++i)
					{
						ImGui::PushID(i);
						ImGui::AlignTextToFramePadding();

						// システムとユーザーで色を分ける
						if (i < 8) ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Layer %2d", i);
						else ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Layer %2d", i);

						ImGui::SameLine(80.0f);

						if (i < 8)
						{
							ImGui::BeginDisabled();
							std::string name = LayerManager::Get().GetLayerName(i);
							char buf[64];
							strcpy_s(buf, name.c_str());
							ImGui::InputText("##Name", buf, sizeof(buf));
							ImGui::EndDisabled();
						}
						else
						{
							std::string name = LayerManager::Get().GetLayerName(i);
							char buf[64];
							strcpy_s(buf, name.c_str());

							ImGui::SetNextItemWidth(250.0f);
							if (ImGui::InputText("##Name", buf, sizeof(buf)))
							{
								LayerManager::Get().SetLayerName(i, buf);
							}

							ImGui::SameLine();
							if (ImGui::Button(ICON_FA_TRASH " Clear"))
							{
								LayerManager::Get().SetLayerName(i, "");
							}
						}
						ImGui::PopID();
					}
					ImGui::EndChild();
					ImGui::EndTabItem();
				}

				// タブ2: 衝突マトリクス
				// ==========================================================
				if (ImGui::BeginTabItem("Collision Matrix"))
				{
					ImGui::TextDisabled("Click a layer name to quick-toggle. Right-click for advanced options.");
					ImGui::Spacing();

					std::vector<int> validLayers;
					for (int i = 0; i < 32; ++i)
					{
						if (LayerManager::Get().IsValidLayer(i)) validLayers.push_back(i);
					}

					int N = (int)validLayers.size();

					ImGui::BeginChild("MatrixRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), true, ImGuiWindowFlags_HorizontalScrollbar);

					if (N > 0 && ImGui::BeginTable("MatrixTable", N + 1, ImGuiTableFlags_BordersInner | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
					{
						// [ヘッダー行]
						ImGui::TableSetupColumn("##RowHeader", ImGuiTableColumnFlags_WidthFixed, 120.0f);
						for (int c = 0; c < N; ++c)
						{
							int layerB = validLayers[N - 1 - c];
							ImGui::TableSetupColumn(std::to_string(layerB).c_str(), ImGuiTableColumnFlags_WidthFixed, 24.0f);
						}
						ImGui::TableHeadersRow();

						for (int c = 0; c < N; ++c)
						{
							ImGui::TableSetColumnIndex(c + 1);
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LayerManager::Get().GetLayerName(validLayers[N - 1 - c]).c_str());
						}

						// [データ行]
						for (int r = 0; r < N; ++r)
						{
							ImGui::TableNextRow();
							int layerA = validLayers[r];

							// 行ヘッダー
							ImGui::TableSetColumnIndex(0);
							ImGui::AlignTextToFramePadding();

							std::string rowLabel = LayerManager::Get().GetLayerName(layerA);

							ImGui::PushID(layerA);
							if (ImGui::Selectable(rowLabel.c_str(), false))
							{
								bool newState = !LayerManager::Get().CanCollide(layerA, layerA);
								for (int c = 0; c < N; ++c) LayerManager::Get().SetCollision(layerA, validLayers[c], newState);
							}
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to toggle all. Right-click for menu.");

							// コンテキストメニュー (右クリック)
							std::string popupID = "RowContext_" + std::to_string(layerA);
							if (ImGui::BeginPopupContextItem(popupID.c_str()))
							{
								ImGui::TextDisabled("Actions for '%s'", rowLabel.c_str());
								ImGui::Separator();
								if (ImGui::MenuItem("Enable All"))
								{
									for (int c = 0; c < N; ++c) LayerManager::Get().SetCollision(layerA, validLayers[c], true);
								}
								if (ImGui::MenuItem("Disable All"))
								{
									for (int c = 0; c < N; ++c) LayerManager::Get().SetCollision(layerA, validLayers[c], false);
								}
								if (ImGui::MenuItem("Invert"))
								{
									for (int c = 0; c < N; ++c)
									{
										bool current = LayerManager::Get().CanCollide(layerA, validLayers[c]);
										LayerManager::Get().SetCollision(layerA, validLayers[c], !current);
									}
								}
								ImGui::Separator();
								if (ImGui::MenuItem("Isolate (Collide self only)"))
								{
									for (int c = 0; c < N; ++c) LayerManager::Get().SetCollision(layerA, validLayers[c], false);
									LayerManager::Get().SetCollision(layerA, layerA, true);
								}
								ImGui::Separator();
								if (ImGui::MenuItem("Copy Mask (Hex)"))
								{
									uint32_t mask = LayerManager::Get().GetCollisionMask(layerA);
									char hexBuf[32];
									snprintf(hexBuf, sizeof(hexBuf), "0x%08X", mask);
									ImGui::SetClipboardText(hexBuf);
								}
								ImGui::EndPopup();
							}
							ImGui::PopID();

							// --- チェックボックス (マトリクス部分) ---
							for (int c = 0; c < N - r; ++c)
							{
								int layerB = validLayers[N - 1 - c];
								ImGui::TableSetColumnIndex(c + 1);

								ImGui::PushID(r * 1000 + c);

								bool canCollide = LayerManager::Get().CanCollide(layerA, layerB);

								float availX = ImGui::GetContentRegionAvail().x;
								float checkWidth = ImGui::GetFrameHeight();
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - checkWidth) * 0.5f);

								if (ImGui::Checkbox("##col", &canCollide))
								{
									LayerManager::Get().SetCollision(layerA, layerB, canCollide);
								}

								if (ImGui::IsItemHovered())
								{
									ImGui::SetTooltip("%s  x  %s", LayerManager::Get().GetLayerName(layerA).c_str(), LayerManager::Get().GetLayerName(layerB).c_str());
								}

								ImGui::PopID();
							}
						}
						ImGui::EndTable();
					}
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}

			ImGui::Separator();
			ImGui::Spacing();

			float btnWidth = 120.0f;
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x - btnWidth - ImGui::GetStyle().WindowPadding.x);
			if (ImGui::Button("Close", ImVec2(btnWidth, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
}
