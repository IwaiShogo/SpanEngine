#pragma once
#include "Runtime/Reflection/SpanReflection.h"

namespace Span
{
	struct Name
	{
		char Value[256];

		Name() { Value[0] = '\0'; }
		Name(const char* name) { strcpy_s(Value, name); }
		Name(const std::string& name) { strcpy_s(Value, name.c_str()); }

		std::string ToString() const { return std::string(Value); }

		// 「ヘッダー部分」で特別表示するため、定義なし
	};
}

