#pragma once
#include "Core/CoreMinimal.h"
#include <imgui.h>

namespace Span
{
	class EditorPanel
	{
	public:
		EditorPanel(const std::string& title) : title(title) {}
		virtual ~EditorPanel() = default;

		// 毎フレーム呼ばれる描画関数
		virtual void OnImGuiRender() = 0;

		// 表示・非表示
		bool IsOpen() const { return isOpen; }
		void Open() { isOpen = true; }
		void Close() { isOpen = false; }

	public:
		std::string title;
		bool isOpen = true;
	};
}
