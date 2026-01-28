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

			if (ImGui::BeginTable("##Vec3Table", 2, ImGuiTableFlags_Resizable))
			{
				// 1列目の幅設定
				ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
				ImGui::TableSetupColumn("##Values", ImGuiTableColumnFlags_None);

				// 行の開始
				ImGui::TableNextRow();

				// --- 1列目: ラベル ---
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", label.c_str());

				// --- 2列目: コントロール ---
				ImGui::TableSetColumnIndex(1);

				// 各項目の幅を計算 (全体幅 / 3)
				float availableWidth = ImGui::GetContentRegionAvail().x;
				float itemSpacing = ImGui::GetStyle().ItemSpacing.x;

				// ボタンサイズ
				float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
				ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

				// 1つの軸(ボタン+入力)の幅
				float axisWidth = (availableWidth - (itemSpacing * 2)) / 3.0f;
				if (axisWidth < buttonSize.x + 20.0f) axisWidth = buttonSize.x + 20.0f; // 最小幅保証

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

				auto DrawAxis = [&](const char* axisLabel, float& value, const ImVec4& color)
					{
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

						// DragFloat
						float dragWidth = axisWidth - buttonSize.x;
						ImGui::SetNextItemWidth(dragWidth);

						std::string dragLabel = std::string("##") + axisLabel;
						if (ImGui::DragFloat(dragLabel.c_str(), &value, 0.1f, 0.0f, 0.0f, "%.2f"))
						{
							changed = true;
						}
					};

				// X Axis
				DrawAxis("X", values.x, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
				ImGui::SameLine();
				ImGui::Dummy(ImVec2{ itemSpacing, 0 }); ImGui::SameLine(); // スペーシング

				// Y Axis
				DrawAxis("Y", values.y, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
				ImGui::SameLine();
				ImGui::Dummy(ImVec2{ itemSpacing, 0 }); ImGui::SameLine();

				// Z Axis
				DrawAxis("Z", values.z, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

				ImGui::PopStyleVar(); // ItemSpacing
				ImGui::EndTable();
			}

			ImGui::PopID();
			return changed;
		}

		// コンポーネント用のヘッダー描画
		static bool DrawComponentHeader(const std::string& name, bool defaultOpen = true)
		{
			ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
			if (defaultOpen) treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

			bool open = ImGui::TreeNodeEx(name.c_str(), treeNodeFlags, name.c_str());

			return open;
		}
	};
}