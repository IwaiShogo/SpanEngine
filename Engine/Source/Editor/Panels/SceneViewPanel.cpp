#include "SceneViewPanel.h"

namespace Span
{
	SceneViewPanel::SceneViewPanel()
		: EditorPanel("Scene View")
	{
	}

	void SceneViewPanel::OnImGuiRender()
	{
		// パディングをゼロにして、ウィンドウいっぱいに画像を表示
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		if (ImGui::Begin(title.c_str(), &isOpen))
		{
			// ウィンドウサイズに合わせて画像を表示
			ImVec2 size = ImGui::GetContentRegionAvail();

			if (textureHandle.ptr != 0)
			{
				ImGui::Image((ImTextureID)textureHandle.ptr, size);
			}
		}
		ImGui::End();

		ImGui::PopStyleVar();
	}
}