#pragma once
#include "ECS/Kernel/Entity.h"

namespace Span
{
	// 親子関係と兄弟関係を管理するコンポーネント (双方向リスト)
	struct Relationship
	{
		Entity Parent = Entity::Null;
		Entity FirstChild = Entity::Null;
		Entity PrevSibling = Entity::Null;
		Entity NextSibling = Entity::Null;	// ヒエラルキーの順序
	};
}

