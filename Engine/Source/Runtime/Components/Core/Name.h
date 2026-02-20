/*****************************************************************//**
 * @file	Name.h
 * @brief	エンティティの名前
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/Containers/FixedString.h"
#include "Runtime/Reflection/SpanReflection.h"

namespace Span
{
	/**
	 * @struct	Name
	 * @brief	🏷️ エンティティに人間が読める名前を付けるコンポーネント。
	 * @details	Editorのヒエラルキービューなどで使用されます。重複可能です。
	 */
	struct Name
	{
		String64 Value;

		Name() = default;
		Name(const char* name) : Value(name) {}
		Name(const std::string& name) : Value(name) {}

		std::string ToString() const { return std::string(Value); }

		bool operator==(const Name& other) const { return Value == other.Value; }

		SPAN_INSPECTOR_BEGIN(Name)
			SPAN_FIELD(Value, HideInInspector())
		SPAN_INSPECTOR_END()
	};
}

