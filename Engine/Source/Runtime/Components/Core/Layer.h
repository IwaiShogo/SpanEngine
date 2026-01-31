#pragma once

namespace Span
{
	// 32ビットビットマスク用 (0-31)
	enum class LayerType
	{
		Default = 0,
		TransparetnFX = 1,
		IgnoreRaycast = 2,
		Water = 3,
		UI = 4,
	};

	struct Layer
	{
		int Value = 0;

		Layer() : Value(0) {}
		Layer(int layerIndex) : Value(layerIndex) {}
	};
}
