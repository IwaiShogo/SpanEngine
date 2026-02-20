/*****************************************************************//**
 * @file	FileDialog.cpp
 * @brief	FileDialogの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "FileDialog.h"
#include "Runtime/Application.h"

#include <commdlg.h>
#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>


namespace Span
{
	std::optional<std::string> FileDialog::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };

		// GLFWウィンドウからHWNDを取得 (Application::Get().GetWindow().GetNativeWindow() を想定)
		// ※ SpanEngineのWindowシステムに合わせて取得方法を調整してください。
		// ここでは一旦NULL(デスクトップを親にする)でフォールバックします。
		HWND hwnd = NULL;
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		if (window) hwnd = glfwGetWin32Window(window);

		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return szFile;
		}
		return std::nullopt;
	}

	std::optional<std::string> FileDialog::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };

		HWND hwnd = NULL;
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		if (window) hwnd = glfwGetWin32Window(window);

		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		// デフォルト拡張子の付与用 (フィルターから抽出することも可能ですが簡易的にspanとします)
		ofn.lpstrDefExt = strchr(filter, '\0') + 1;

		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return szFile;
		}
		return std::nullopt;
	}
}
