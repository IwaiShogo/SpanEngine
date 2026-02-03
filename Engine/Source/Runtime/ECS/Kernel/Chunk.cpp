#include "Chunk.h"
#include "Archetype.h"
#include "Entity.h"

namespace Span
{
	Chunk::Chunk(uint32 capacity)
		: Count(0), Capacity(capacity)
	{
		// 16KBのメモリを確保
		Memory = static_cast<uint8*>(std::malloc(CHUNK_SIZE));
	}

	Chunk::~Chunk()
	{
		if (Memory)
		{
			std::free(Memory);
			Memory = nullptr;
		}
	}

	void Chunk::MoveEntityData(Archetype* arch, uint32 srcIndex, uint32 destIndex)
	{
		if (srcIndex == destIndex) return;

		// 1. EntityIDの移動
		EntityID* ids = reinterpret_cast<EntityID*>(Memory);
		ids[destIndex] = ids[srcIndex];

		// 2. 各コンポーネントの移動
		for (ComponentTypeID typeID : arch->GetTypes())
		{
			size_t offset = arch->GetComponentOffset(typeID);
			size_t size = arch->GetComponentSize(typeID);

			uint8* base = Memory + offset;
			uint8* src = base + (srcIndex * size);
			uint8* dest = base + (destIndex * size);

			memcpy(dest, src, size);
		}
	}
}
