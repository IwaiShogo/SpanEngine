#pragma once
#include "ECS/Kernel/Entity.h"

namespace Span
{
	// 親を持つEntityに付与するコンポーネント
	// ※子は親を知っているが、親は子を知らない
	struct Parent
	{
		Entity Value;

		Parent() : Value(Entity::Null) {}
		Parent(Entity parent) : Value(parent) {}
	};
}
