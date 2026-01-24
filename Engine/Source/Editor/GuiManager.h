#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS

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

		static void ApplyStyle();

	private:
		static ComPtr<ID3D12DescriptorHeap> srvHeap;
	};
}
