/*****************************************************************//**
 * @file	Archetype.h
 * @brief	ECSのメモリレイアウト定義 (アーキタイプ) を行うクラス。
 * 
 * @details	
 * アーキタイプは、「同じコンポーネントの組み合わせ」を持つエンティティのグループです。
 * データの物理的な配置 (メモリレイアウト) を決定する設計図の役割を果たします。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "ECS/Internal/ComponentType.h"
#include "Chunk.h"

namespace Span
{
	/**
	 * @class	ArchetypeSignature
	 * @brief	🔑 アーキタイプを識別するための署名キー
	 * 
	 * @details
	 * 持っているコンポーネントIDのリストをソートして保持します。
	 * これにより、追加順序に関わらず「構成が同じなら同じ署名」とみなされます。
	 */
	class ArchetypeSignature
	{
	public:
		/**
		 * @brief	コンポーネントIDを追加して再ソートします。
		 */
		void Add(ComponentTypeID typeID)
		{
			componentTypes.push_back(typeID);
			std::sort(componentTypes.begin(), componentTypes.end());
		}

		/**
		 * @brief	指定したコンポーネントIDを削除します。
		 */
		void Remove(ComponentTypeID typeID)
		{
			// erase-remove idiom
			auto it = std::remove(componentTypes.begin(), componentTypes.end(), typeID);
			if (it != componentTypes.end())
			{
				componentTypes.erase(it, componentTypes.end());
			}
		}

		/**
		 * @brief	指定したコンポーネントが含まれているか判定します。
		 */
		bool Has(ComponentTypeID typeID) const
		{
			return std::binary_search(componentTypes.begin(), componentTypes.end(), typeID);
		}

		/// @brief Operators
		/// @{
		bool operator<(const ArchetypeSignature& other) const
		{
			return componentTypes < other.componentTypes;
		}
		bool operator==(const ArchetypeSignature& other) const
		{
			return componentTypes == other.componentTypes;
		}
		/// @}

		const std::vector<ComponentTypeID>& GetTypes() const { return componentTypes; }

	private:
		std::vector<ComponentTypeID> componentTypes;
	};

	/**
	 * @class	Archetype
	 * @brief	🏢 エンティティのコンテナクラス。メモリレイアウトを管理します。
	 * 
	 * @details
	 * 1つのアーキタイプは複数の `Chunk` (16KBブロック) を持ち、
	 * コンポーネントデータを **SoA (Structure of Arrays)** 形式で格納します。
	 * 
	 * ### 🧠 メモリレイアウト (Chunk Memory Layout)
	 * 例: `Transform` (Vec3) と `Velocity` (Vec3) を持つアーキタイプの場合
	 * | Header | EntityIDs[]    | Transform[]          | Velocity[]           | Padding |
	 * | :---:  | :---           | :---                 | :---                 | :--- |
	 * | ...    | `[0][1][2]...` | `[Pos][Pos][Pos]...` | `[Vel][Vel][Vel]...` | ... |
	 * 
	 * これにより、同じコンポーネントへの連続アクセスがキャッシュフレンドリーになります。
	 */
	class Archetype
	{
	public:
		/**
		 * @brief	コンストラクタ。メモリレイアウトを計算します。
		 * @param	types コンポーネント型IDのリスト
		 * @param	sizes 各コンポーネントのサイズ
		 * @param	alignments 各コンポーネントのアライメント要件
		 */
		Archetype(const std::vector<ComponentTypeID>& types,
			const std::vector<size_t>& sizes,
			const std::vector<size_t>& alignments);

		~Archetype();

		SPAN_NON_COPYABLE(Archetype);

		// 🔍 Queries
		// ============================================================

		/**
		 * @brief	指定したコンポーネントを持っているか確認します。
		 */
		bool HasComponent(ComponentTypeID typeID) const
		{
			return signature.Has(typeID);
		}

		/**
		 * @brief	要求された全てのコンポーネントを持っているか確認します。
		 * @param	queryTypes 要求するコンポーネントIDリスト
		 * @return	全て持っていれば true
		 */
		bool HasAllComponents(const std::vector<ComponentTypeID>& queryTypes) const
		{
			// 署名と比較して、queryTypesが全部含まれているか確認
			for (ComponentTypeID id : queryTypes)
			{
				if (!signature.Has(id)) return false;
			}
			return true;
		}

		// 💾 Memory Management
		// ============================================================

		/**
		 * @brief	新しいEntity用のスペースを確保します。
		 * 
		 * @details
		 * 空きのあるChunkを探し、なければ新しいChunkを確保します。
		 * @param	entityID 割り当てるエンティティID
		 * @return	Chunk内でのインデックス
		 */
		uint32 AllocateEntity(EntityID entityID);

		/**
		 * @brief	指定したチャンク内のエンティティデータを削除し、末尾の要素で穴埋めします。
		 * @param	chunk 対象のチャンクポインタ
		 * @param	index チャンク内での削除対象のインデックス
		 * @return	移動によってインデックスが変わったエンティティのID。穴埋めが発生しなかった場合は無効なID。
		 */
		EntityID RemoveEntity(Chunk* chunk, uint32 index);

		/**
		 * @brief	コンポーネント配列の「チャンク内オフセット」を取得します。
		 * @param	typeID コンポーネント型ID
		 * @return	チャンク先頭からのバイトオフセット
		 */
		size_t GetComponentOffset(ComponentTypeID typeID) const;

		/**
		 * @brief	コンポーネントの単体サイズを取得します。
		 */
		size_t GetComponentSize(ComponentTypeID typeID) const;

		/**
		 * @brief	コンポーネントのアライメントを取得します。
		 */
		size_t GetComponentAlignment(ComponentTypeID typeID) const;

		// 📊 Getters
		// ============================================================

		/**
		 * @brief	このアーキタイプが管理している全チャンクのリスト
		 */
		const std::vector<Chunk*>& GetChunks() const { return chunks; }

		/**
		 * @brief	1つのチャンクに格納できるエンティティ最大数
		 */
		uint32 GetChunkCapacity() const { return chunkCapacity; }

		/**
		 * @brief	構成コンポーネントの型リスト
		 */
		const std::vector<ComponentTypeID>& GetTypes() const { return typeIDs; }

	private:
		ArchetypeSignature signature;	///< コンポーネントの構成の署名

		// --- Layout Info ---
		std::vector<ComponentTypeID> typeIDs;						///< TypeIDリスト
		std::unordered_map<ComponentTypeID, size_t> typeOffsets;	///< TypeID -> Chunk内オフセット
		std::unordered_map<ComponentTypeID, size_t> typeSizes;		///< TypeID -> サイズ (バイト)
		std::unordered_map<ComponentTypeID, size_t> typeAlignments;	///< TypeID -> アライメント

		size_t entitySize = 0;										///< Entity1体あたりの合計サイズ (バイト)
		uint32 chunkCapacity = 0;									///< 1チャンクに何体入るか

		// --- Storage ---
		std::vector<Chunk*> chunks;									///< 確保されたメモリブロック群
	};
}
