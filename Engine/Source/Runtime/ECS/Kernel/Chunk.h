#pragma once
#include "Core/CoreMinimal.h"
#include "Entity.h"

namespace Span
{
	// チャンク1つあたりのサイズ (16KB)
	constexpr size_t CHUNK_SIZE = 16 * 1024;

	/**
	 * @brief チャンク (Chunk)
	 * コンポーネントデータを格納する固定サイズのメモリブロック。
	 * 1つのチャンクには、同じアーキタイプのEntityデータだけが入ります。
	 */
	struct Chunk
	{
		// 生メモリブロック (16KB)
		uint8* Memory = nullptr;

		// 現在格納されているEntity数
		uint32 Count = 0;

		// このチャンクに格納できる最大数
		uint32 Capacity = 0;

		// 所属するアーキタイプへのポインタ
		class Archetype* OwnerArchetype = nullptr;

		Chunk(uint32 capacity);
		~Chunk();

		// 指定したコンポーネント配列の先頭アドレスを取得する
		void* GetBuffer(size_t offset) const
		{
			return Memory + offset;
		}

		// データの移動
		void MoveEntityData(Archetype* arch, uint32 srcIndex, uint32 destIndex);
	};
}