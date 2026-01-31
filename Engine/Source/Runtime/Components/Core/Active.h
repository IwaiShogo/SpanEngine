#pragma once

namespace Span
{
	struct Active
	{
		bool Value = true;

		Active() = default;
		Active(bool isActive) : Value(isActive) {}
	};
}
