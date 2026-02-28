#include "MemoryArena.h"

namespace Span
{
	MemoryArena::~MemoryArena()
	{
		Shutdown();
	}

	void MemoryArena::Initialize(size_t sizeInBytes)
	{
		// 既に確保済みなら一旦捨てる
		if (memoryBlock != nullptr)
		{
			Shutdown();
		}

		// mallocでOSから生メモリをもらう
		memoryBlock = static_cast<uint8*>(std::malloc(sizeInBytes));
		totalSize = sizeInBytes;
		usedOffset = 0;

		SPAN_LOG("MemoryArena Initialized: %zu bytes", totalSize);
	}

	void MemoryArena::Shutdown()
	{
		if (memoryBlock)
		{
			std::free(memoryBlock);
			memoryBlock = nullptr;
		}
		totalSize = 0;
		usedOffset = 0;
	}

	void* MemoryArena::AllocateRaw(size_t size, size_t alignment)
	{
		// 現在のポインタ位置
		size_t currentAddress = reinterpret_cast<size_t>(memoryBlock) + usedOffset;

		// アライアメント調整: アドレスが alignment の倍数になるように調整する
		// 例: 4バイト境界 -> アドレス末尾が 00, 04, 08, 0C になるようにずらす
		size_t padding = 0;
		size_t mask = alignment - 1;
		if ((currentAddress & mask) != 0)
		{
			padding = alignment - (currentAddress & mask);
		}

		// 必要な合計サイズ（本体 + パディング）
		size_t totalNeeded = size + padding;

		// メモリ不足チェック
		if (usedOffset + totalNeeded > totalSize)
		{
			SPAN_ERROR("MemoryArena Overflow! Need: %zu, Left: %zu", totalNeeded, totalSize - usedOffset);
			return nullptr;
		}

		// ポインタ計算
		size_t alignedAddress = currentAddress + padding;

		// オフセットを進める
		usedOffset += totalNeeded;

		return reinterpret_cast<void*>(alignedAddress);
	}

	void MemoryArena::Reset()
	{
		usedOffset = 0;
	}
}

