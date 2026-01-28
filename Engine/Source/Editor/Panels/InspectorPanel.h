#pragma once
#include "EditorPanel.h"
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
