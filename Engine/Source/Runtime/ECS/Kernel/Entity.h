/*****************************************************************//**
 * @file	Entity.h
 * @brief	ECSにおけるオブジェクトの識別子 (ID) を定義します。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	/**
	 * @struct	EntityID
	 * @brief	エンティティの内部ID構造 (64-bit)。
	 *
	 * @details
	 * エンティティIDは、配列インデックスと再利用世代 (Generation) を組み合わせたものです。
	 *
	 * ### 📊 ビットレイアウト (Bit Layout)
	 * | Bit Range | Name         | Description |
	 * | :---      | :---         | :--- |
	 * | 0-31      | **Index**    | エンティティ配列へのアクセサ。高速なルックアップに使用。 |
	 * | 32-63     | **Generation | IDの再利用カウンタ。古い参照が無効なEntityにアクセスするのを防ぐ。 |
	 */
	struct EntityID
	{
		uint32 Index;		///< @brief	配列のインデックス
		uint32 Generation;	///< @brief 世代番号（再利用対策）

		/// @name	比較演算子
		/// @{
		bool operator==(const EntityID& other) const { return Index == other.Index && Generation == other.Generation; }
		bool operator!=(const EntityID& other) const { return !(*this == other); }

		bool operator<(const EntityID& other) const
		{
			if (Index != other.Index) return Index < other.Index;
			return Generation < other.Generation;
		}
		/// @}
	};

	/// @brief	Null (無効) として扱うための定数ID
	static const EntityID NullEntityID = { UINT32_MAX, 0 };

	/**
	 * @class	Entity
	 * @brief	📦 ゲーム内のあらゆるオブジェクトを表すハンドルクラス。
	 *
	 * @details
	 * Entity自体はデータを持たず、単なる **64bitのID** です
	 * 実際のデータ (コンポーネント) は `World` クラスが管理する `Chunk` 内に存在します。
	 *
	 * ### 📝 使い方
	 * ```cpp
	 * // Entityを作成
	 * Entity e = world.CreateEntity<Transform, MeshRenderer>();
	 * // 有効チェック
	 * if (e != Entity::Null)
	 * {
	 *     // ...
	 * }
	 * ```
	 */
	struct Entity
	{
		/// @brief	内部ID
		EntityID ID = NullEntityID;

		/// @brief	無効なエンティティを表す静的変数
		static const Entity Null;

		// Constructors
		// ============================================================

		Entity() = default;
		Entity(EntityID id) : ID(id) {}

		// Utilities
		// ============================================================

		/**
		 * @brief	このエンティティが無効 (Null) かどうかを判定します。
		 * @retval	true	無効なエンティティ
		 * @retval	false	有効なエンティティ
		 */
		bool IsNull() const { return ID.Index == UINT32_MAX; }

		/**
		 * @brief	ログ出力用などで数値としてIDを取得したい場合に使用します。
		 * @return	64bit整数にパックされたID
		 */
		uint64 ToUInt64() const { return (static_cast<uint64>(ID.Generation) << 32) | ID.Index; }

		// Operators
		// ============================================================

		bool operator==(const Entity& other) const { return ID == other.ID; }
		bool operator!=(const Entity& other) const { return ID != other.ID; }
		bool operator<(const Entity& other) const { return ID < other.ID; }
	};

	// 実体定義
	inline const Entity Entity::Null = { NullEntityID };
}

// ハッシュマップ
namespace std
{
	/**
	 * @brief	Entityをキーにするためのハッシュ関数特殊化
	 */
	template <>
	struct hash<Span::EntityID>
	{
		std::size_t operator()(const Span::EntityID& id) const
		{
			// IndexとGenerationを混ぜてハッシュ値を作る
			return hash<uint32_t>()(id.Index) ^ (hash<uint32_t>()(id.Generation) << 1);
		}
	};
}

