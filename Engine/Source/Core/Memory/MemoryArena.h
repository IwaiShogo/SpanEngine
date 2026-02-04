/*****************************************************************//**
 * @file	MemoryArena.h
 * @brief	線形メモリ割り当て (Linear Allocator)。
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
	 * @class	MemoryArena
	 * @brief	🧠 高速なメモリ割り当てを行うアリーナクラス。
	 * 
	 * @details
	 * 初期化時に巨大なメモリブロックを一度だけ確保し、`Allocate` 呼び出し時にはポインタを進めるだけでメモリを返します。
	 * 個別の解放はサポートせず、`Reset`で全体を一括解放します。
	 * 一時的な作業用メモリや、フレームごとのデータ生成に最適です。
	 */
	class MemoryArena
	{
	public:
		MemoryArena() = default;
		~MemoryArena();

		// コピー禁止
		SPAN_NON_COPYABLE(MemoryArena);

		/// @brief	指定サイズ(byte)のメモリブロックを確保します。
		void Initialize(size_t sizeInBytes);

		/// @brief	確保したメモリをOSに返却します。
		void Shutdown();

		/**
		 * @brief	型 `T` のオブジェクト用メモリを割り当てます。
		 * @tparam	T 確保する型
		 * @param	count 配列要素数 (Default: 1)
		 * @return	確保されたメモリへのポインタ
		 */
		template <typename T>
		T* Allocate(size_t count = 1)
		{
			return reinterpret_cast<T*>(AllocateRaw(sizeof(T) * count, alignof(T)));
		}

		/**
		 * @brief	生のバイト数を指定してメモリを割り当てます。
		 * @param	size 必要バイト数
		 * @param	alignment アライメント要件
		 */
		void* AllocateRaw(size_t size, size_t alignment);

		/// @brief	ポインタを先頭に戻し、全メモリを「解放済み」とみなします。
		void Reset();

		/// @brief	現在の使用量を取得
		size_t GetUsedMemory() const { return usedOffset; }
		/// @brief	全体の容量を取得
		size_t GetTotalSize() const { return totalSize; }

	private:
		uint8* memoryBlock = nullptr;	// 生メモリの先頭ポインタ
		size_t totalSize = 0;			// 全容量
		size_t usedOffset = 0;			// 現在どこまで使ったか
	};
}

