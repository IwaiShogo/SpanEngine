#pragma once
#include "EditorPanel.h"

namespace Span
{
	class SceneViewPanel : public EditorPanel
	{
	public:
		SceneViewPanel();

		void OnImGuiRender() override;

		// 表示するテクスチャ
		void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle) { textureHandle = handle; }

	private:
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = {};
	};
}
