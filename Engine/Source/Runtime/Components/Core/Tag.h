/*****************************************************************//**
 * @file	Tag.h
 * @brief	エンティティのタグ分類
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
	 * @struct	Tag
	 * @brief	🔖 エンティティを検索・分類するための文字列タグ。
	 * @details	"Player", "Enemy", "MainCamera" などの文字列を設定します。
	 */
	struct Tag
	{
		String64 Value = "Untagged";

		// Constructors
		// ============================================================

		Tag() = default;
		Tag(const std::string& tag) : Value(tag) {}

		bool operator==(const Tag& other) const { return Value == other.Value; }

		SPAN_INSPECTOR_BEGIN(Tag)
		SPAN_INSPECTOR_END()
	};
}

