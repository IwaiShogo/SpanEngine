/*****************************************************************//**
 * @file	EnvironmentPanel.cpp
 * @brief	EnvironmentPanelの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "EnvironmentPanel.h"
#include "Editor/PanelManager.h"
#include "Runtime/Application.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Span
{
	static AutoRegisterPanel<EnvironmentPanel> _reg_env("EnvironmentPanel");

	EnvironmentPanel::EnvironmentPanel()
		: EditorPanel("Environment Settings")
	{
		isOpen = false;
	}

	void EnvironmentPanel::OnImGuiRender()
	{
		// ウィンドウの初期サイズを設定
		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &isOpen))
		{
			// 現在のアクティブシーンから環境設定を取得
			auto& env = Application::Get().GetActiveScene().Environment;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

			// --- Skybox Settings ---
			if (ImGui::CollapsingHeader("Skybox Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10.0f);

				const char* modeStrings[] = { "Procedural (3-Color Gradient)", "HDRI (High Dynamic Range Image)" };
				int currentMode = static_cast<int>(env.Mode);
				if (ImGui::Combo("Sky Mode", &currentMode, modeStrings, IM_ARRAYSIZE(modeStrings)))
				{
					env.Mode = static_cast<SkyboxMode>(currentMode);
				}

				ImGui::Spacing();

				if (env.Mode == SkyboxMode::Procedural)
				{
					ImGuiColorEditFlags hdrFlags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;
					ImGui::ColorEdit3("Top Color", &env.SkyTopColor.x, hdrFlags);
					ImGui::ColorEdit3("Horizon Color", &env.SkyHorizonColor.x, hdrFlags);
					ImGui::ColorEdit3("Bottom Color", &env.SkyBottomColor.x, hdrFlags);

					if (ImGui::Button("Reset Colors"))
					{
						env.SkyTopColor = { 0.35f, 0.5f, 0.7f };
						env.SkyHorizonColor = { 0.7f, 0.75f, 0.8f };
						env.SkyBottomColor = { 0.2f, 0.2f, 0.2f };
					}
				}
				else if (env.Mode == SkyboxMode::HDRI)
				{
					char pathBuffer[256];
					strcpy_s(pathBuffer, env.HDRIPath.c_str());

					ImGui::Text("HDRI Asset Path");
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
					if (ImGui::InputText("##HDRIPath", pathBuffer, sizeof(pathBuffer)))
					{
						env.HDRIPath = pathBuffer;
					}

					ImGui::SameLine();
					if (ImGui::Button("X", ImVec2(30, 0))) { env.HDRIPath = ""; }

					// D&Dサポート
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							const wchar_t* path = (const wchar_t*)payload->Data;
							std::wstring ws(path);
							env.HDRIPath = std::string(ws.begin(), ws.end());
						}
						ImGui::EndDragDropTarget();
					}
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Tip: Drop .hdr file here");
				}

				ImGui::Unindent(10.0f);
			}

			// --- Global Lighting Settings ---
			if (ImGui::CollapsingHeader("Global Lighting", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10.0f);
				ImGui::SliderFloat("Camera Exposure", &env.Exposure, 0.1f, 10.0f, "%.2f");
				ImGui::SliderFloat("Ambient Intensity", &env.AmbientIntensity, 0.0f, 5.0f, "%.2f");
				ImGui::SliderFloat("Reflection Intensity", &env.EnvReflectionIntensity, 0.0f, 10.0f, "%.2f");
				ImGui::Unindent(10.0f);
			}

			ImGui::PopStyleVar();
		}
		ImGui::End();
	}
}
