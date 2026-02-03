#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	/**
	 * @brief	線形メモリ割り当てクラス（Linear Allocator）
	 * 事前に巨大なブロックを確保し、ポインタを進めるだけで割り当てる。
	 * 解放は「全リセット」のみ対応することで、極限の速度を実現する。
	 */
	class MemoryArena
	{
	public:
		MemoryArena() = default;
		~MemoryArena();

		// コピー禁止
		SPAN_NON_COPYABLE(MemoryArena);

		// 初期化: 指定したサイズ(byte)のメモリブロックをOSから確保する
		void Initialize(size_t sizeInBytes);

		// 解放: OSにメモリを返却する
		void Shutdown();

		// 割り当て: Arenaからメモリを切り出して返す
		template <typename T>
		T* Allocate(size_t count = 1)
		{
			return reinterpret_cast<T*>(AllocateRaw(sizeof(T) * count, alignof(T)));
		}

		// 割り当て: 生のバイト数指定
		void* AllocateRaw(size_t size, size_t alignment);

		// リセット: ポインタを先頭に戻す
		void Reset();

		// 現在の使用量を取得
		size_t GetUsedMemory() const { return usedOffset; }
		// 全体の容量を取得
		size_t GetTotalSize() const { return totalSize; }

	private:
		uint8* memoryBlock = nullptr;	// 生メモリの先頭ポインタ
		size_t totalSize = 0;			// 全容量
		size_t usedOffset = 0;			// 現在どこまで使ったか
	};
}

