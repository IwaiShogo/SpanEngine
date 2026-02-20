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
	/**
	 * @struct	Layer
	 * @brief	🍰 オブジェクトが所属するレイヤー番号 (0~31)。
	 * @details
	 * 物理衝突の判定マトリクスや、カメラの加リングマスク (描画対象選別) に使用されます。
	 */
	struct Layer
	{
		uint8_t Value = 0;

		Layer() = default;
		Layer(uint8_t val) : Value(val) {}

		SPAN_INSPECTOR_BEGIN(Layer)
			SPAN_FIELD(Value, HideInInspector())
		SPAN_INSPECTOR_END()
	};
}

