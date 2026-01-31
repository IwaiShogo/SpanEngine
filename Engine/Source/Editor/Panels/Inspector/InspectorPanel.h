#pragma once
#include "Editor/Panels/EditorPanel.h"
#include "ECS/Kernel/Entity.h"

namespace Span
{
	class InspectorPanel : public EditorPanel
	{
	public:
		InspectorPanel() : EditorPanel("Inspector") {}

		void OnImGuiRender() override;
	};
}
