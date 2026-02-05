/*****************************************************************//**
 * @file	MeshRenderer.h
 * @brief	メッシュの描画設定とマテリアル参照。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Resources/Material.h"
#include "Runtime/Reflection/SpanReflection.h"

namespace Span
{
	/**
	 * @struct	MeshRenderer
	 * @brief	🖌 オブジェクトを画面に描画するための設定コンポーネント。
	 * 
	 * @details
	 * `MeshFilter` で指定された形状に対し、どの `Material` (色・質感) を適用するか決定します。
	 * また、影のキャスト/レシーブなどの描画フラグも管理します。
	 */
	struct MeshRenderer
	{
		Material* material = nullptr;
		bool castShadows = true;
		bool receiveShadows = true;

		MeshRenderer() = default;
		MeshRenderer(Material* m) : material(m) {}

		SPAN_INSPECTOR_BEGIN(MeshRenderer)
			// 1. Material Info
			// マテリアルのパラメータをここで簡易編集できるようにする。
			// マテリアルエディタとして別ウィンドウにするので簡易実装
			// 2. Settings
			SPAN_FIELD(castShadows)
			SPAN_FIELD(receiveShadows)
		SPAN_INSPECTOR_END()
	};
}

