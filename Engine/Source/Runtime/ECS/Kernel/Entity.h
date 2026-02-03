#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	// IDの内部構造（合計64bit）
	struct EntityID
	{
		uint32 Index;		// 配列のインデックス
		uint32 Generation;	// 世代番号（再利用対策）

		// 比較演算子
		bool operator==(const EntityID& other) const { return Index == other.Index && Generation == other.Generation; }
		bool operator!=(const EntityID& other) const { return !(*this == other); }

		bool operator<(const EntityID& other) const
		{
			if (Index != other.Index) return Index < other.Index;
			return Generation < other.Generation;
		}
	};

	// nullとして扱うための定数
	static const EntityID NullEntityID = { UINT32_MAX, 0 };

	/**
	 * @brief	Entityハンドル
	 * ユーザーが扱うのはこのクラス。実体はIDのみを持つ軽量オブジェクト
	 */
	struct Entity
	{
		EntityID ID = NullEntityID;

		static const Entity Null;

		// 無効なEntityかどうか
		bool IsNull() const { return ID.Index == UINT32_MAX; }

		// 比較演算子
		bool operator==(const Entity& other) const { return ID == other.ID; }
		bool operator!=(const Entity& other) const { return ID != other.ID; }
		bool operator<(const Entity& other) const { return ID < other.ID; }

		// ログ出力などで数字としてほしい時用
		uint64 ToUInt64() const { return (static_cast<uint64>(ID.Generation) << 32) | ID.Index; }
	};

	// 実体定義
	inline const Entity Entity::Null = { NullEntityID };
}

namespace std
{
	// EntityIDをキーとして使えるようにハッシュ化を定義
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

