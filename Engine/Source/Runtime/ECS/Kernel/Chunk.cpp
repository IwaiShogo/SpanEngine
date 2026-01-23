#include "Chunk.h"

namespace Span
{
	Chunk::Chunk(uint32 capacity)
		: Count(0), Capacity(capacity)
	{
		// 16KB‚Ìƒƒ‚ƒŠ‚ğŠm•Û
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
}