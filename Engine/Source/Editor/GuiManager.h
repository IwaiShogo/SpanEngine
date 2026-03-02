/*****************************************************************//**
 * @file	GuiManager.h
 * @brief	ImGuiのライフサイクルとレンダリング管理。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

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
	/**
	 * @class	GuiManager
	 * @brief	🎨 エディタUI (ImGui)の統括マネージャー。
	 * 
	 * @details
	 * ImGuiの初期化、フレーム開始/終了処理、および各エディタパネルの描画ループを管理します。
	 * また、DirectX 12のディスクリプタヒープ(SRV)を管理し、テクスチャをImGuiで表示するための登録機能も提供します。
	 */
	class GuiManager
	{
	public:
		/**
		 * @brief	ImGuiコンテキストとバックエンドを初期化します。
		 * @param	hWnd ウィンドウハンドル
		 * @param	device D3D12デバイス
		 * @param	commandQueue コマンドキュー
		 * @param	numFrames バックバッファ数 (通常2)
		 */
		static void Initialize(HWND hWnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue,int numFrames);

		/// @brief	ImGuiのshutdown処理を行います。
		static void Shutdown();

		/**
		 * @brief	ImGuiのフレームを開始します。
		 * 
		 * @details
		 * `ImGui()::NewFrame()` を呼び出し、入力のキャプチャ状態を更新します。
		 * ゲームループの更新処理(Update)の前に呼び出す必要があります。
		 */
		static void BeginFrame();

		/**
		 * @brief	ImGuiの描画コマンドを発行し、フレームを終了します。
		 * @param	commandList 描画に使用するコマンドリスト
		 * 
		 * @details
		 * 登録されている全てのパネルの `OnImGuiRender()` を呼び出し、
		 * 最終的な頂点データをコマンドリストに積みます。
		 */
		static void EndFrame(ID3D12GraphicsCommandList* commandList);

		// Panel Management
		// ============================================================

		/// @brief	管理対象のエディタパネルを追加します。
		static void AddPanel(std::shared_ptr<EditorPanel> panel);

		/**
		 * @brief	特定の方のパネルを取得します。
		 * @tparam	T 取得したいパネルクラス (EditorPanelを継承)
		 * @return	パネルへのポインタ。見つからない場合は nullptr。
		 */
		template<typename T>
		static std::shared_ptr<T> GetPanel()
		{
			for (auto& panel : panels)
			{
				if (auto p = std::dynamic_pointer_cast<T>(panel)) return p;
			}
			return nullptr;
		}

		/**
		 * @brief	テクスチャをImGuiで表示するために登録します。
		 * @details
		 * ImGui用にSRVヒープにディスクリプタをコピーし、GPUハンドルを返します。
		 * 返されたハンドルは `ImGui::Image((ImTextureID)handle.ptr, ...)` で使用できます。
		 * 
		 * @param	srcHandle コピー元のCPUディスクリプタハンドル
		 * @param	isDynamic 毎フレーム内容が変わる・再生成されるリソースは場合は true を指定
		 * @return	ImGuiで使用可能なGPUディスクリプタハンドル
		 */
		static D3D12_GPU_DESCRIPTOR_HANDLE RegisterTexture(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, bool isDynamic = false);

	private:
		/// @brief	フォントや配色などのスタイル設定を適用します。
		static void ApplyStyle();

	private:
		static ComPtr<ID3D12DescriptorHeap> srvHeap;				///< ImGui専用のSRVヒープ
		static std::vector<std::shared_ptr<EditorPanel>> panels;	///< 管理中のパネルリスト

		static ID3D12Device* m_device;
	};
}

