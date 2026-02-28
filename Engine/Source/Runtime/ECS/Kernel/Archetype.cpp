#include "Archetype.h"

namespace Span
{
	Archetype::Archetype(const std::vector<ComponentTypeID>& types,
		const std::vector<size_t>& sizes,
		const std::vector<size_t>& alignments)
		: typeIDs(types)
	{
		// signature にもIDを登録する
		for (ComponentTypeID id : types)
		{
			signature.Add(id);
		}

		// 1. キャパシティの初期見積もり
		// ------------------------------------------------------------
		// 1エンティティ当たりの単純合計サイズを計算
		size_t totalBytesSimple = sizeof(EntityID);
		for (size_t size : sizes)
		{
			totalBytesSimple += size;
		}
		entitySize = totalBytesSimple;

		// 余裕をもって少し少なめに見積もる
		chunkCapacity = static_cast<uint32>((CHUNK_SIZE * 0.9f) / entitySize);
		if (chunkCapacity == 0) chunkCapacity = 1;

		// 2. パディング考慮のリサイズ (Safe Size Calculation)
		// ------------------------------------------------------------
		// アライメントによって必要なメモリが増えるため、16KBに収まるまでキャパシティを減らす
		while (chunkCapacity > 0)
		{
			size_t currentOffset = 0;

			// EntityID配列 (先頭)
			currentOffset += sizeof(EntityID) * chunkCapacity;

			bool fits = true;

			// 各コンポーネント配列
			for (size_t i = 0; i < types.size(); ++i)
			{
				size_t align = alignments[i];
				size_t size = sizes[i];

				// パディング計算
				if (currentOffset % align != 0)
				{
					currentOffset += align - (currentOffset % align);
				}

				// 配置後の末尾位置
				size_t endOffset = currentOffset + (size * chunkCapacity);

				// 16KBを超えたらアウト
				if (endOffset > CHUNK_SIZE)
				{
					fits = false;
					break;
				}

				// 次のコンポーネントの為にオフセットを進める
				currentOffset = endOffset;
			}

			// 全て収まったらループ終了
			if (fits)
			{
				break;
			}

			// 収まらないなら個数を減らして再計算
			chunkCapacity--;
		}

		if (chunkCapacity == 0) chunkCapacity = 1;

		// 3. 確定したキャパシティでオフセット計算
		// ------------------------------------------------------------
		size_t finalOffset = 0;

		// 先頭はEntityIDの配列
		finalOffset += sizeof(EntityID) * chunkCapacity;

		// コンポーネントごとの配列開始位置を決定
		for (size_t i = 0; i < types.size(); ++i)
		{
			size_t align = alignments[i];
			if (finalOffset % align != 0)
			{
				finalOffset += align - (finalOffset % align);
			}

			// 本来はここでアライメント調整(Padding)を入れるべきだが、簡易実装
			typeOffsets[types[i]] = finalOffset;
			// サイズを保存
			typeSizes[types[i]] = sizes[i];
			// アライメント
			typeAlignments[types[i]] = alignments[i];

			// 次のコンポーネントのためにオフセットを進める (サイズ * キャパシティ)
			finalOffset += sizes[i] * chunkCapacity;
		}
	}

	Archetype::~Archetype()
	{
		for (Chunk* chunk : chunks)
		{
			delete chunk;
		}
		chunks.clear();
	}

	uint32 Archetype::AllocateEntity(EntityID entityID)
	{
		Chunk* targetChunk = nullptr;

		// 空きがあるチャンクを探す（または末尾を見る）
		if (!chunks.empty())
		{
			Chunk* lastChunk = chunks.back();
			if (lastChunk->Count < lastChunk->Capacity)
			{
				targetChunk = lastChunk;
			}
		}

		// 空きがなければ新規作成
		if (!targetChunk)
		{
			targetChunk = new Chunk(chunkCapacity);
			targetChunk->OwnerArchetype = this;
			chunks.push_back(targetChunk);
		}

		// EntityIDを書き込む
		uint32 index = targetChunk->Count;

		// チャンクの先頭はEntityID配列と決まっている
		EntityID* ids = reinterpret_cast<EntityID*>(targetChunk->Memory);
		ids[index] = entityID;

		targetChunk->Count++;

		return index;
	}

	EntityID Archetype::RemoveEntity(Chunk* chunk, uint32 index)
	{
		// 範囲外アy不正なチャンクなら何もしない
		if (!chunk || index >= chunk->Count) return EntityID{ 0, 0 };

		uint32 lastIndex = chunk->Count - 1;
		EntityID* ids = reinterpret_cast<EntityID*>(chunk->Memory);
		EntityID movedEntityID = { 0, 0 };

		// 削除対象が末尾の要素でなければ、末尾のデータを削除対象の位置へコピー
		if (index < lastIndex)
		{
			movedEntityID = ids[lastIndex];

			// 1. EntityIDの配列を上書き
			ids[index] = movedEntityID;

			// 2. 各コンポーネントのデータを上書き
			for (size_t i = 0; i < typeIDs.size(); ++i)
			{
				ComponentTypeID typeID = typeIDs[i];
				size_t offset = typeOffsets[typeID];
				size_t size = typeSizes[typeID];

				// 対象コンポーネント配列の先頭ポインタ
				uint8_t* componentArray = chunk->Memory + offset;

				// コピー先とコピー元のアドレス計算
				uint8_t* dest = componentArray + (size * index);
				uint8_t* src = componentArray + (size * lastIndex);

				std::memcpy(dest, src, size);
			}
		}

		// 要素数を減らす
		chunk->Count--;

		return movedEntityID;
	}

	size_t Archetype::GetComponentOffset(ComponentTypeID typeID) const
	{
		auto it = typeOffsets.find(typeID);
		return (it != typeOffsets.end()) ? it->second : 0;
	}

	size_t Archetype::GetComponentSize(ComponentTypeID typeID) const
	{
		auto it = typeSizes.find(typeID);
		return (it != typeSizes.end()) ? it->second : 0;
	}

	size_t Archetype::GetComponentAlignment(ComponentTypeID typeID) const
	{
		auto it = typeAlignments.find(typeID);
		return (it != typeAlignments.end()) ? it->second : 0;
	}
}
