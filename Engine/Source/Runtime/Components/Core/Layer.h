/*****************************************************************//**
 * @file	Layer.h
 * @brief	物理演算やレンダリングのレイヤー分け。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Runtime/Reflection/SpanReflection.h"

namespace Span
{
	/// @brief	32ビットビットマスク用のレイヤー定数 (0-31)
	enum class LayerType
	{
		Default = 0,
		TransparetnFX = 1,
		IgnoreRaycast = 2,
		Water = 3,
		UI = 4,
	};

	/**
	 * @struct	Layer
	 * @brief	🍰 オブジェクトが所属するレイヤー番号 (0~31)。
	 * @details
	 * 物理衝突の判定マトリクスや、カメラの加リングマスク (描画対象選別) に使用されます。
	 */
	struct Layer
	{
		int Value = 0;

		Layer() : Value(0) {}
		Layer(int layerIndex) : Value(layerIndex) {}
	};
}

