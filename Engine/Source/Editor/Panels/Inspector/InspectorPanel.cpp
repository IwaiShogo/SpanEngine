#include "InspectorPanel.h"
#include "Editor/SelectionManager.h"
#include "Runtime/Application.h"
#include "Runtime/Reflection/ComponentRegistry.h"
#include "Core/Math/SpanMath.h"
#include "Editor/PanelManager.h"
#include "Editor/ImGui/ImGuiUI.h"

// 基本コンポーネント
#include "Runtime/Components/Core/Name.h"
#include "Runtime/Components/Core/Tag.h"
#include "Runtime/Components/Core/Layer.h"
#include "Runtime/Components/Core/Transform.h"
#include "Runtime/Components/Core/Active.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Span
{
	static AutoRegisterPanel<InspectorPanel> _reg("Inspector");

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Inspector", &isOpen);

		// 1. 選択ターゲットの取得
		Entity selected = SelectionManager::GetPrimary();
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
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f - 10);

			// タグリスト (将来的にはTagManagerから取得)
			const char* tags[] = { "Untagged", "Player", "Enemy", "MainCamera" };
			int currentTagIndex = 0;

			// 現在のタグを取得
			if (Tag* t = world.GetComponentPtr<Tag>(selected))
			{
				for (int i = 0; i < IM_ARRAYSIZE(tags); i++) {
					if (t->Value == tags[i]) { currentTagIndex = i; break; }
				}
			}
			else
			{
				// 持っていなければ追加してUntaggedにする
				world.AddComponent<Tag>(selected, Tag("Untagged"));
			}

			if (ImGui::Combo("##Tag", &currentTagIndex, tags, IM_ARRAYSIZE(tags)))
			{
				// 変更を適用
				if (Tag* t = world.GetComponentPtr<Tag>(selected)) {
					t->Value = tags[currentTagIndex];
				}
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
				if (meta->Name == "Name" || meta->Name == "Tag" || meta->Name == "Layer" || meta->Name == "Transform" || meta->Name == "Active") continue;

				if (ImGui::MenuItem(meta->Name.c_str()))
				{
					// AddComponent...
				}
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}
}
