#pragma once
#include "Core/Math/SpanMath.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace Span
{
	class ImGuiUI
	{
	public:
		// XYZカラー付きベクトル編集 (Table API版)
		static bool DrawVec3Control(const std::string& label, Vector3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
		{
			bool changed = false;
			ImGui::PushID(label.c_str());

			// Columns APIを使用 (BeginTableよりクラッシュしにくい)
			ImGui::Columns(2);

			// 1列目の幅設定
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::Text("%s", label.c_str());
			ImGui::NextColumn();

			// 2列目: X, Y, Z の描画
			ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

			float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
			ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

			auto DrawAxis = [&](const char* axisLabel, float& value, const ImVec4& color)
				{
					// 軸ごとのIDスコープを作る (これがないと全て同じIDになり連動してしまう)
					ImGui::PushID(axisLabel);

					ImGui::PushStyleColor(ImGuiCol_Button, color);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ color.x + 0.1f, color.y + 0.1f, color.z + 0.1f, 1.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

					if (ImGui::Button(axisLabel, buttonSize))
					{
						value = resetValue;
						changed = true;
					}
					ImGui::PopStyleColor(3);

					ImGui::SameLine();

					// IDは "label/axisLabel/##Drag" となり一意になる
					if (ImGui::DragFloat("##Drag", &value, 0.1f, 0.0f, 0.0f, "%.2f"))
					{
						changed = true;
					}
					ImGui::PopItemWidth();

					ImGui::PopID(); // axisLabelのIDをポップ
				};

			// X Axis
			DrawAxis("X", values.x, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
			ImGui::SameLine();

			// Y Axis
			DrawAxis("Y", values.y, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
			ImGui::SameLine();

			// Z Axis
			DrawAxis("Z", values.z, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

			ImGui::PopStyleVar(); // ItemSpacing
			ImGui::Columns(1); // カラムリセット

			ImGui::PopID();
			return changed;
		}

		// コンポーネント用のヘッダー描画
		static bool DrawComponentHeader(const std::string& name, bool& isRemoved, bool defaultOpen = true)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

			ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
			if (defaultOpen) treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

			bool open = ImGui::TreeNodeEx(name.c_str(), treeNodeFlags, name.c_str());

			// 右クリックメニュー (TreeNodeの直後に配置することでヘッダー全体で反応する)
			if (ImGui::BeginPopupContextItem("ComponentSettings_Context", ImGuiPopupFlags_MouseButtonRight))
			{
				ImGui::TextDisabled("%s", name.c_str());
				ImGui::Separator();
				if (ImGui::MenuItem("Remove Component"))
				{
					isRemoved = true;
				}
				ImGui::EndPopup();
			}

			// --- 歯車アイコン (右端) ---
			ImGui::SameLine();
			float buttonWidth = 20.0f;
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonWidth - 5.0f);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // 透明

			if (ImGui::Button(":", ImVec2(buttonWidth, 0)))
			{
				ImGui::OpenPopup("ComponentSettings");
			}
			ImGui::PopStyleColor();

			// 歯車ボタン用のポップアップ
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove Component"))
				{
					isRemoved = true;
				}
				ImGui::EndPopup();
			}

			ImGui::PopStyleVar();
			return open;
		}
	};
}
