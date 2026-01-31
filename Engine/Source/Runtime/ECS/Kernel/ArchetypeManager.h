#pragma once
#include "Core/CoreMinimal.h"
#include "Archetype.h"

namespace Span
{
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

		// 型リスト T... (例: Position, Velocity) に対応するアーキタイプを取得または作成する
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

		const std::map<ArchetypeSignature, Archetype*>& GetAllArchetypes() const { return archetypes; }

	private:
		// 署名をキーにしてアーキタイプを保存
		std::map<ArchetypeSignature, Archetype*> archetypes;
	};
}
