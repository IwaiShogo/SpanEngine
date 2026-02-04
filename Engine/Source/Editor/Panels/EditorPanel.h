/*****************************************************************//**
 * @file	EditorPanel.h
 * @brief	エディタ上のウィンドウ (パネル) の基底クラス。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include <imgui.h>

namespace Span
{
	/**
	 * @class	EditorPanel
	 * @brief	🖼 全てのエディタパネルの基底クラス。
	 * 
	 * @details
	 * 新しいパネルを作る際は、このクラスを継承し、 `OnImGuiRender` を実装してください。
	 * ImGuiの `Begin()` と `End()` は、通常 `OnImGuiRender` の内部で呼び出します。
	 */
	class EditorPanel
	{
	public:
		/**
		 * @brief	コンストラクタ。
		 * @param	title ウィンドウのタイトルバーに表示される名前
		 */
		EditorPanel(const std::string& title) : title(title) {}
		virtual ~EditorPanel() = default;

		/**
		 * @brief	毎フレーム呼ばれる描画関数。
		 * @details	ここに `ImGui::Begin()`, UIウィジット, `ImGui::End()` を記述します。
		 */
		virtual void OnImGuiRender() = 0;

		// 👁 Visibility Control
		// ============================================================

		/// @brief	パネルが表示中かどうか
		bool IsOpen() const { return isOpen; }

		/// @brief	パネルを表示状態にします。
		void Open() { isOpen = true; }

		/// @brief	パネルを非表示状態にします。
		void Close() { isOpen = false; }

	public:
		std::string title;	///< パネルタイトル
		bool isOpen = true;	///< 表示フラグ
	};
}

