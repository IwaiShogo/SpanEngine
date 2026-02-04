/*****************************************************************//**
 * @file	Chunk.h
 * @brief	ECSのメモリブロック (チャンク) 定義。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Entity.h"

namespace Span
{
	/// @brief		1つのチャンクが確保する固定メモリサイズ (16KB)
	/// @details	L1/L2キャッシュへの適合率を高めるために設定されています。
	constexpr size_t CHUNK_SIZE = 16 * 1024;

	/**
	 * @struct	Chunk
	 * @brief	🧱 コンポーネントデータを格納する固定サイズのメモリブロック。
	 * 
	 * @details
	 * Span EngineのECSにおけるメモリ割り当ての最小単位です。
	 * 1つのチャンクには、**同じアーキタイプ** (同じコンポーネント構成) のエンティティデータのみが格納されます。
	 * 
	 * ### 🧠 メモリ構造 (Memory Layout)
	 * | Header | EntityIDs[]   | ComponentA[]      | ComponentB[]      | ... |
	 * | :---:  | :---          | :---              | :---              | :--- |
	 * | (N/A)  | `[ID][ID]...` | `[Data][Data]...` | `[Data][Data]...` | ... |
	 * 
	 * このSoA (Structure of Arrays) 配置により、SIMDによる並列処理やキャッシュプリフェッチが最適化されます。
	 */
	struct Chunk
	{
		/// @brief	生メモリブロック (16KB)
		uint8* Memory = nullptr;

		/// @brief	現在格納されているEntity数
		uint32 Count = 0;

		/// @brief	このチャンクに格納できる最大数
		uint32 Capacity = 0;

		/// @brief	所属するアーキタイプへのポインタ
		class Archetype* OwnerArchetype = nullptr;

		Chunk(uint32 capacity);
		~Chunk();

		/**
		 * @brief	指定したオフセット位置にあるバッファの先頭アドレスを取得します。
		 * @param	offset チャンク先頭からのバイトオフセット
		 * @return	バッファへのポインタ
		 */
		void* GetBuffer(size_t offset) const
		{
			return Memory + offset;
		}

		/**
		 * @brief	チャンク内のエンティティデータを移動 (コピー) します。
		 * @details	配列の穴埋め (Swap-back削除) などに使用されます。
		 * @param	arch データのレイアウト情報を持つアーキタイプ
		 * @param	srcIndex 移動元のインデックス
		 * @param	destIndex 移動先のインデックス
		 */
		void MoveEntityData(Archetype* arch, uint32 srcIndex, uint32 destIndex);
	};
}
