#pragma once

namespace Span
{
	struct Tag
	{
		std::string Value = "Untagged";
		Tag() = default;
		Tag(const std::string& tag) : Value(tag) {}
	};
}
