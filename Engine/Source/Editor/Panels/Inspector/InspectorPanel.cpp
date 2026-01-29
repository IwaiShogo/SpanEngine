#include "InspectorPanel.h"
#include "Editor/SelectionManager.h"
#include "Runtime/Application.h"
#include "Runtime/Reflection/ComponentRegistry.h"
#include "Core/Math/SpanMath.h"
#include "Editor/PanelManager.h"

#include <imgui.h>

namespace Span
{
	static AutoRegisterPanel<InspectorPanel> _reg("Inspector");

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Inspector", &isOpen);

		Entity selected = SelectionManager::GetPrimary();
		World& world = Application::Get().GetWorld();

		if (selected.IsNull() || !world.IsAlive(selected))
		{
			ImGui::Text("No Entity Selected");
			ImGui::End();
			return;
		}

		ImGui::TextDisabled("ID: %d | Gen: %d", selected.ID.Index, selected.ID.Generation);
		ImGui::Separator();
		ImGui::Spacing();

		// --- コンポーネントの並び替えと描画 ---

		// 1. レジストリからリストをコピー
		std::vector<ComponentMetadata> components = ComponentRegistry::GetAll();

		// 2. ソート (Transformを最優先、あとは名前順)
		std::sort(components.begin(), components.end(), [](const ComponentMetadata& a, const ComponentMetadata& b)
			{
				if (a.Name == "Transform") return true; // aが先
				if (b.Name == "Transform") return false; // bが先

				// それ以外は名前順
				return a.Name < b.Name;
			});

		// 3. 描画
		for (const auto& meta : components)
		{
			meta.DrawFunc(selected, world);
		}

		// コンポーネント追加ボタン
		ImGui::Spacing();
		ImGui::Separator();
		if (ImGui::Button("Add Component", ImVec2(-1, 0)))
		{
			ImGui::OpenPopup("AddComponentPopup");
		}

		// ポップアップの中身もソートして表示すると親切
		if (ImGui::BeginPopup("AddComponentPopup"))
		{
			// ここでも同じリストを使える
			for (const auto& meta : components)
			{
				// 既に持っているコンポーネントは表示しない等の制御も将来的に入れる
				if (ImGui::MenuItem(meta.Name.c_str()))
				{
					// AddComponentの実装はまだリフレクションに入っていないため
					// フェーズ2-3以降で実装します
				}
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}
}