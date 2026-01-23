#pragma once
#include "Core/CoreMinimal.h"
#include "ECS/Internal/ComponentType.h"
#include "Chunk.h"

namespace Span
{
	/**
	 * @brief	アーキタイプ署名 (Signature)
	 * コンポーネントIDのリストをソートして保持したもの。
	 * これが一致すれば「同じアーキタイプ」とみなす。
	 */
	class ArchetypeSignature
	{
	public:
		void Add(ComponentTypeID typeID)
		{
			componentTypes.push_back(typeID);
			std::sort(componentTypes.begin(), componentTypes.end());
		}

		void Remove(ComponentTypeID typeID)
		{
			// erase-remove idiom
			auto it = std::remove(componentTypes.begin(), componentTypes.end(), typeID);
			if (it != componentTypes.end())
			{
				componentTypes.erase(it, componentTypes.end());
			}
		}

		bool Has(ComponentTypeID typeID) const
		{
			return std::binary_search(componentTypes.begin(), componentTypes.end(), typeID);
		}

		// マップのキーにするために比較演算子
		bool operator<(const ArchetypeSignature& other) const
		{
			return componentTypes < other.componentTypes;
		}
		bool operator==(const ArchetypeSignature& other) const
		{
			return componentTypes == other.componentTypes;
		}

		const std::vector<ComponentTypeID>& GetTypes() const { return componentTypes; }

	private:
		std::vector<ComponentTypeID> componentTypes;
	};

	/**
	 * @brief アーキタイプ
	 * 同じコンポーネント構成を持つEntity群を管理する。
	 * データの「型」だけでなく、実際の「メモリ(Chunk)」も管理する。
	 */
	class Archetype
	{
	public:
		// コンストラクタでレイアウトを計算する
		Archetype(const std::vector<ComponentTypeID>& types,
			const std::vector<size_t>& sizes,
			const std::vector<size_t>& alignments);

		~Archetype();

		SPAN_NON_COPYABLE(Archetype);

		// このアーキタイプは特定のコンポーネント
		bool HasComponent(ComponentTypeID typeID) const
		{
			return signature.Has(typeID);
		}

		// 複数のコンポーネントを全て持っているか判定
		bool HasAllComponents(const std::vector<ComponentTypeID>& queryTypes) const
		{
			// 署名と比較して、queryTypesが全部含まれているか確認
			for (ComponentTypeID id : queryTypes)
			{
				if (!signature.Has(id)) return false;
			}
			return true;
		}

		// 新しいEntity分のスペースを確保し、そのインデックスを返す
		uint32 AllocateEntity(EntityID entityID);

		// コンポーネントIDから、その配列の「チャンク内オフセット」を取得
		size_t GetComponentOffset(ComponentTypeID typeID) const;

		// このアーキタイプが持つ全チャンク
		const std::vector<Chunk*>& GetChunks() const { return chunks; }

		// 1チャンクあたりの収容数
		uint32 GetChunkCapacity() const { return chunkCapacity; }

		// 型リストを取得
		const std::vector<ComponentTypeID>& GetTypes() const { return typeIDs; }

	private:
		ArchetypeSignature signature;

		// 構成要素
		std::vector<ComponentTypeID> typeIDs;

		// メモリレイアウト情報
		std::unordered_map<ComponentTypeID, size_t> typeOffsets;	// TypeID -> Chunk内オフセット
		size_t entitySize = 0;										// Entity1体あたりの合計サイズ (バイト)
		uint32 chunkCapacity = 0;									// 1チャンクに何体入るか

		// データの実体
		std::vector<Chunk*> chunks;
	};
}