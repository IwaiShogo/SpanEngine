/*****************************************************************//**
 * @file	EnvironmentPanel.h
 * @brief	シーンの環境設定を編集するパネル
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Editor/Panels/EditorPanel.h"

namespace Span
{
	/**
	 * @class	EnvironmentPanel
	 * @brief	⛅ シーンの環境光、Skybox、露出などを調整するエディタウィンドウ。
	 */
	class EnvironmentPanel : public EditorPanel
	{
	public:
		EnvironmentPanel();

		void OnImGuiRender() override;
	};
}
