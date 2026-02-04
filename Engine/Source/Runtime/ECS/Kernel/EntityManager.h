/*****************************************************************//**
 * @file	EntityManager.h
 * @brief	エンティティIDのライフサイクル管理クラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Entity.h"

namespace Span
{
	/**
	 * @class	EntityManager
	 * @brief	🆔 Entity IDの発行、破棄、有効性チェックを行う管理クラス。
	 * 
	 * @details
	 * 配列のインデックスと「世代番号(Generation)」を組み合わせることで、
	 * 削除されたIDが再利用された際に、古い参照が無効であることを正しく検出できるようにします (ABA問題の回避)。
	 * 
	 * ### 🔄 ライフサイクル
	 * 1. **Create**: 空きインデックスがあれば再利用し、なければ新規発行。
	 * 2. **Destroy**: 世代番号をインクリメントし、インデックスを空きリストへ返却。
	 * 3. **IsAlive**: 現在の世代番号と、IDの世代番号が一致するかチェック。
	 */
	class EntityManager
	{
	public:
		EntityManager() = default;
		~EntityManager() = default;

		SPAN_NON_COPYABLE(EntityManager);

		/// @brief	新しいEntityを作成して返す
		Entity CreateEntity();

		/// @brief	Entityを削除する
		void DestroyEntity(Entity entity);

		/**
		 * @brief	Entityが現在も有効 (生存している) かを確認します。
		 * @param	entity 確認対象のハンドル
		 * @return	true: 生存 / false: 破棄済み
		 */
		bool IsAlive(Entity entity) const;

		/// @brief	現在アクティブな (生きている) エンティティの総数
		size_t GetActiveEntityCount() const { return activeCount; }

	private:
		/// 各スロットの現在の世代番号を管理する配列
		std::vector<uint32> generations;

		/// 再利用待ちのインデックスリスト
		std::deque<uint32> freeIndices;

		/// 生存数
		size_t activeCount = 0;

		/// 最低限のインデックス管理数
		static constexpr uint32 MINIMUM_FREE_INDICES = 1024;
	};
}

