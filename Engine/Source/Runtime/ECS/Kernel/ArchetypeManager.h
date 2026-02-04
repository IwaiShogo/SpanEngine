/*****************************************************************//**
 * @file	ArchetypeManager.h
 * @brief	アーキタイプの生成と管理を行うクラス
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Archetype.h"

namespace Span
{
	/**
	 * @class	ArchetypeManager
	 * @brief	🏭 アーキタイプファクトリー&キャッシュ。
	 *
	 * @details
	 * コンポーネントの組み合わせごとに一意の `Archetype` インスタンスを管理します。
	 * 同じ組み合わせ (例: Transform + Velocity) が要求された場合、
	 * 新しく作成せず、既存のキャッシュされたアーキタイプを返します。
	 */
	class ArchetypeManager
	{
	public:
		~ArchetypeManager()
		{
			for (auto& pair : archetypes)
			{
				delete pair.second;
			}
			archetypes.clear();
		}

		/**
		 * @brief	テンプレート引数からアーキタイプを取得または作成します。
		 *
		 * @tparam	ComponentTypes コンポーネントの型リスト (可変長)
		 * @return	対象のArchetypeポインタ
		 *
		 * @code	{.cpp}
		 * Archetype* arch = manager.GetOrCreateArchetype<Transform, MeshRenderer>();
		 * @endcode
		 */
		template <typename... ComponentTypes>
		Archetype* GetOrCreateArchetype()
		{
			// 1. 要求されてコンポーネント構成の署名(Signature)を作る
			ArchetypeSignature signature;

			// C++17 Fold Expression: 引数パックを展開して全部Addする
			(signature.Add(ComponentType<ComponentTypes>::GetID()), ...);

			// 2. 既に存在すればそれを返す
			auto it = archetypes.find(signature);
			if (it != archetypes.end())
			{
				return it->second;
			}

			// 3. なければ新規作成
			// 型ID、サイズ、アライメントのリストを作成
			std::vector<ComponentTypeID> typeIDs = { ComponentType<ComponentTypes>::GetID()... };
			std::vector<size_t> sizes = { sizeof(ComponentTypes)... };
			std::vector<size_t> alignments = { alignof(ComponentTypes)... };

			Archetype* newArchetype = new Archetype(typeIDs, sizes, alignments);
			archetypes[signature] = newArchetype;

			SPAN_LOG("Created new Archetype. Signature size: %zu", typeIDs.size());

			return newArchetype;
		}

		/**
		 * @brief	動的なIDリストからアーキタイプを取得または作成します。
		 * @details	AddComponent/RemoveComponentなど、型情報が動的に変わる場合に使用します。
		 */
		Archetype* GetOrCreateArchetype(
			const std::vector<ComponentTypeID>& typeIDs,
			const std::vector<size_t>& sizes,
			const std::vector<size_t>& alignments)
		{
			ArchetypeSignature signature;
			for (auto id : typeIDs)
			{
				signature.Add(id);
			}

			if (auto it = archetypes.find(signature); it != archetypes.end())
			{
				return it->second;
			}

			Archetype* newArchetype = new Archetype(typeIDs, sizes, alignments);
			archetypes[signature] = newArchetype;
			return newArchetype;
		}

		/// @brief	管理している全アーキタイプを取得 (イテレーション用)
		const std::map<ArchetypeSignature, Archetype*>& GetAllArchetypes() const { return archetypes; }

	private:
		// 署名をキーにしてアーキタイプを保存
		std::map<ArchetypeSignature, Archetype*> archetypes;
	};
}

