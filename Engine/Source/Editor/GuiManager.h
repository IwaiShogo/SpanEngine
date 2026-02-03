#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "Panels/EditorPanel.h"
#include "Panels/SceneView/SceneViewPanel.h"

#include "Core/CoreMinimal.h"
#include "Graphics/Renderer.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

namespace Span
{
	class GuiManager
	{
	public:
		static void Initialize(HWND hWnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue,int numFrames);
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame(ID3D12GraphicsCommandList* commandList);

		// パネル管理
		static void AddPanel(std::shared_ptr<EditorPanel> panel);

		// 特定のパネルを取得するヘルパー
		template<typename T>
		static std::shared_ptr<T> GetPanel()
		{
			for (auto& panel : panels)
			{
				if (auto p = std::dynamic_pointer_cast<T>(panel)) return p;
			}
			return nullptr;
		}

		static D3D12_GPU_DESCRIPTOR_HANDLE RegisterTexture(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

	private:
		static void ApplyStyle();

		static ComPtr<ID3D12DescriptorHeap> srvHeap;
		static std::vector<std::shared_ptr<EditorPanel>> panels;

		static ID3D12Device* m_device;
	};
}

