#include "Archetype.h"

namespace Span
{
	Archetype::Archetype(const std::vector<ComponentTypeID>& types,
		const std::vector<size_t>& sizes,
		const std::vector<size_t>& alignments)
		: typeIDs(types)
	{
		// 1. レイアウト計算
		// Chunk構造: [EntityID配列] [CompA配列] [CompB配列] ...

		// signature にもIDを登録する
		for (ComponentTypeID id : types)
		{
			signature.Add(id);
		}

		// まずEntityIDの分を確保
		size_t totalBytesPerEntity = sizeof(EntityID);

		// 各コンポーネントのサイズを足していく
		for (size_t size : sizes)
		{
			totalBytesPerEntity += size;
		}
		entitySize = totalBytesPerEntity;

		// 1チャンク(16KB)に何体入るか計算
		chunkCapacity = static_cast<uint32>(CHUNK_SIZE / entitySize);

		// 最低1体は入るように保証
		if (chunkCapacity == 0) chunkCapacity = 1;

		// 2. オフセット計算 (SoA配置) & サイズ保存
		// Memory: [EntityIDs (Cap個)] [CompA (Cap個)] [CompB (Cap個)] ...

		size_t currentOffset = 0;

		// 先頭はEntityIDの配列
		currentOffset += sizeof(EntityID) * chunkCapacity;

		// コンポーネントごとの配列開始位置を決定
		for (size_t i = 0; i < types.size(); ++i)
		{
			// 本来はここでアライメント調整(Padding)を入れるべきだが、簡易実装
			typeOffsets[types[i]] = currentOffset;
			// サイズを保存
			typeSizes[types[i]] = sizes[i];
			// アライメント
			typeAlignments[types[i]] = alignments[i];

			// 次のコンポーネントのためにオフセットを進める (サイズ * キャパシティ)
			currentOffset += sizes[i] * chunkCapacity;
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