/*****************************************************************//**
 * @file	RelationshipSystem.h
 * @brief	エンティティ間の親子・兄弟関係 (階層構造) を操作するシステム。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date   2026/02/05	初回作成日
 * 			作業内容：	- 追加：
 *
 * @update	2026/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#pragma once
#include "ECS/Kernel/System.h"
#include "Components/Core/Relationship.h"
#include "Components/Core/Transform.h"
#include "Components/Core/LocalToWorld.h"

namespace Span
{
	/**
	 * @class	RelationshipSystem
	 * @brief	🔗 階層構造 (リンクリスト) の操作ロジックを提供する静的ヘルパークラス。
	 *
	 * @details
	 * Entity自体はデータを持たないため、このクラスの静的関数を通じて `Relationship` コンポーネントを操作し、
	 * 親子付けや切り離しを行います。
	 *
	 * ### 🌳 リンクリスト構造
	 * ```text
	 * [Parent]
	 * | FirstChild
	 * v
	 * [Child A] <-> [Child B] <-> [Child C] ...
	 * (Prev/Next)   (Prev/Next)
	 * ```
	 */
	class RelationshipSystem : public System
	{
	public:
		/**
		 * @brief	指定したエンティティを現在の親から切断し、孤立させます。
		 * @param	world ワールドポインタ
		 * @param	entity 接続対象のエンティティ
		 *
		 * @details
		 * 兄弟間のリンク (Prev/Next) を繋ぎ直し、親の `FirstChild` が自分だった場合は次の兄弟に更新します。
		 * 実行後はルート階層 (親なし) になります。
		 */
		static void Disconnect(World* world, Entity entity)
		{
			Relationship& rel = world->GetComponent<Relationship>(entity);
			Entity parent = rel.Parent;
			Entity prev = rel.PrevSibling;
			Entity next = rel.NextSibling;

			// 前の兄弟の Next を更新
			if (!prev.IsNull())
			{
				world->GetComponent<Relationship>(prev).NextSibling = next;
			}
			// 自分が長男なら、親の FirstChild を次の兄弟にする
			else if (!parent.IsNull())
			{
				world->GetComponent<Relationship>(parent).FirstChild = next;
			}

			// 次の兄弟の Prev を更新
			if (!next.IsNull())
			{
				world->GetComponent<Relationship>(next).PrevSibling = prev;
			}

			// 自分のリンク情報をクリア
			rel.Parent = Entity::Null;
			rel.PrevSibling = Entity::Null;
			rel.NextSibling = Entity::Null;
		}

		/**
		 * @brief	親子関係を設定します (末尾に追加)。
		 * @param	world ワールドポインタ
		 * @param	child 子にするエンティティ
		 * @param	parent 親にするエンティティ (Nullの場合はルート化)
		 */
		static void SetParent(World* world, Entity child, Entity parent)
		{
			Relationship& childRel = world->GetComponent<Relationship>(child);

			// 既にその親なら何もしない
			if (childRel.Parent == parent && parent.IsNull()) return;

			// 1. 現在の繋がりを切る
			Disconnect(world, child);

			// 2. 新しい親に接続
			childRel.Parent = parent;

			if (!parent.IsNull())
			{
				Relationship& newParentRel = world->GetComponent<Relationship>(parent);
				Entity firstChild = newParentRel.FirstChild;

				if (firstChild.IsNull())
				{
					// 最初の子
					newParentRel.FirstChild = child;
				}
				else
				{
					// リストの末尾に追加
					Entity current = firstChild;
					while (true)
					{
						Relationship& currentRel = world->GetComponent<Relationship>(current);
						if (currentRel.NextSibling.IsNull())
						{
							currentRel.NextSibling = child;
							childRel.PrevSibling = current;
							break;
						}
						current = currentRel.NextSibling;
					}
				}
			}
		}

		/**
		 * @brief	特定の兄弟の「直前」に挿入します (順序変更用)。
		 * @param	world ワールドポインタ
		 * @param	child 移動させるエンティティ
		 */
		static void InsertBefore(World* world, Entity child, Entity targetSibling, Entity parentIfTargetIsNull = Entity::Null)
		{
			if (child == targetSibling) return;

			// ターゲットがNullの場合 (末尾追加)
			if (targetSibling.IsNull())
			{
				if (parentIfTargetIsNull.IsNull()) return;

				// 既存のSetParentを使えば末尾に追加される
				SetParent(world, child, parentIfTargetIsNull);
				return;
			}

			Relationship& targetRel = world->GetComponent<Relationship>(targetSibling);
			Entity parent = targetRel.Parent;

			// 1. 切断
			Disconnect(world, child);

			// 2. 親を設定
			Relationship& childRel = world->GetComponent<Relationship>(child);
			childRel.Parent = parent;

			// 3. 挿入処理
			Entity prev = targetRel.PrevSibling;

			if (prev.IsNull())
			{
				// targetが長男だった場合 -> childが新長男になる
				if (!parent.IsNull())
				{
					world->GetComponent<Relationship>(parent).FirstChild = child;
				}
				childRel.NextSibling = targetSibling;
				targetRel.PrevSibling = child;
			}
			else
			{
				// prev <-> child <-> target の形に繋ぐ
				Relationship& prevRel = world->GetComponent<Relationship>(prev);

				prevRel.NextSibling = child;
				childRel.PrevSibling = prev;

				childRel.NextSibling = targetSibling;
				targetRel.PrevSibling = child;
			}
		}
	};
}

