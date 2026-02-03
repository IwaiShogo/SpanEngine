#pragma once
#include "Editor/Panels/EditorPanel.h"
#include "ECS/Kernel/Entity.h"

namespace Span
{
	class HierarchyPanel : public EditorPanel
	{
	public:
		HierarchyPanel() : EditorPanel("Hierarchy") {}

		void OnImGuiRender() override;

	private:
		// ツリー描画の再帰関数
		void DrawEntityNode(Entity entity);

		// 右クリックメニュー
		void DrawContextMenu(Entity entity);

		// ルート階層 (親無し) の右クリックメニュー
		void DrawEmptySpaceContextMenu();

		// ドラッグ & ドロップ処理
		void HandleDragDrop(Entity targetEntity);
	};
}

