/*****************************************************************//**
 * @file	Relationship.h
 * @brief	ECSにおける階層構造 (親子関係) データ。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "ECS/Kernel/Entity.h"

namespace Span
{
	/**
	 * @struct	Relationship
	 * @brief	🔗 エンティティ間の親子関係を管理するリンクリスト。
	 * 
	 * @details
	 * ECSで階層構造を表現するための標準的な手法です。
	 * ユーザーが直接IDを書き換えるのではなく、`HierarchyPanel` や `RelationshipSystem` を通じて操作します。
	 */
	struct Relationship
	{
		Entity Parent = Entity::Null;		///< 親エンティティ
		Entity FirstChild = Entity::Null;	///< 最初の子エンティティ
		Entity PrevSibling = Entity::Null;	///< 前の兄弟
		Entity NextSibling = Entity::Null;	///< 次の兄弟

		SPAN_INSPECTOR_BEGIN(Relationship)
			SPAN_FIELD(Parent, HideInInspector())
			SPAN_FIELD(FirstChild, HideInInspector())
			SPAN_FIELD(PrevSibling, HideInInspector())
			SPAN_FIELD(NextSibling, HideInInspector())
		SPAN_INSPECTOR_END()
	};
}

