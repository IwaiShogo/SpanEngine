#pragma once
#include "Core/CoreMinimal.h"
#include "EntityManager.h"
#include "ArchetypeManager.h"
#include "System.h"

namespace Span
{
	// エンティティのデータの「住所」を示す構造体
	struct EntityLocation
	{
		Archetype* PtrArchetype;	// どのアーキタイプか
		Chunk* PtrChunk;			// どのチャンクか
		uint32 IndexInChunk;		// チャンク内の何番目か
	};

	class World
	{
	public:
		World() = default;
		~World() = default;

		SPAN_NON_COPYABLE(World);

		// --- Entity作成 ---
		template <typename... ComponentTypes>
		Entity CreateEntity()
		{
			// 1. IDを発行
			Entity entity = entityManager.CreateEntity();

			// 2. 適切なアーキタイプを取得
			Archetype* archetype = archetypeManager.GetOrCreateArchetype<ComponentTypes...>();

			// 3. アーキタイプ内のチャンクに場所を確保
			uint32 index = archetype->AllocateEntity(entity.ID);

			// チャンクは末尾に追加されたもの
			Chunk* targetChunk = archetype->GetChunks().back();
			targetChunk = GetTargetChunk(archetype, index);

			// 4. 住所録に登録
			EntityLocation loc;
			loc.PtrArchetype = archetype;
			loc.PtrChunk = targetChunk;
			loc.IndexInChunk = index;

			entityLocationMap[entity.ID] = loc;

			// 5. コンポーネントの初期化
			InitializeComponents<ComponentTypes...>(loc);

			return entity;
		}

		// --- ForEach ---

		/**
		 * @brief	指定したコンポーネントを持つすべてのEntityに対して関数を実行する
		 * @tparam	ComponentType 要求するコンポーネント (例: Transform, LocalToWorld)
		 * @param	func実行するラムダ式 [](Transform& t, LocalToWorld& ltw) { ... }
		 */
		template <typename... ComponentTypes, typename Func>
		void ForEach(Func&& func)
		{
			// 1. 検索対象の型IDリストを作成
			std::vector<ComponentTypeID> queryTypes = { ComponentType<ComponentTypes>::GetID()... };

			// 2. 全アーキタイプを走査
			const auto& allArchetypes = archetypeManager.GetAllArchetypes();

			for (const auto& pair : allArchetypes)
			{
				Archetype* arch = pair.second;

				// 3. 条件に合うアーキタイプか
				if (!arch->HasAllComponents(queryTypes))
				{
					continue;
				}

				// 4. チャンクごとの処理
				auto offsets = std::make_tuple(arch->GetComponentOffset(ComponentType<ComponentTypes>::GetID())...);

				for (Chunk* chunk : arch->GetChunks())
				{
					if (chunk->Count == 0) continue;

					// 5. 各コンポーネント配列の先頭ポインタを取得して実行
					ProcessChunk<ComponentTypes...>(chunk, chunk->Count, offsets, func);
				}
			}
		}

		// --- コンポーネント取得 (読み書き用) ---
		template <typename T>
		T& GetComponent(Entity entity)
		{
			// 1. 生存確認 & コンポーネント所持確認
			if (!entityManager.IsAlive(entity))
			{
				SPAN_ERROR("GetComponent failed: Entity is dead.");
				static T dummy{};
				return dummy;
			}

			auto it = entityLocationMap.find(entity.ID);
			if (it == entityLocationMap.end())
			{
				SPAN_ERROR("GetComponent failed: Entity not found in map.");
				static T dummy{};
				return dummy;
			}

			EntityLocation& loc = it->second;

			// 2. このアーキタイプがコンポーネントTを持っているか
			size_t offset = loc.PtrArchetype->GetComponentOffset(ComponentType<T>::GetID());
			if (offset == 0 && ComponentType<T>::GetID() != loc.PtrArchetype->GetTypes()[0])
			{
				// オフセット0は通常EntityID配列だが、もしTが最初のコンポーネントでないなら「持っていない」と判定
			}

			// 3. メモリ計算: Chunkの先頭 + 配列オフセット + (サイズ * インデックス)
			uint8* componentArrayBase = loc.PtrChunk->Memory + offset;
			T* componentArray = reinterpret_cast<T*>(componentArrayBase);

			return componentArray[loc.IndexInChunk];
		}

		// --- コンポーネント設定 (値渡し) ---
		template <typename T>
		void SetComponent(Entity entity, const T& value)
		{
			T& component = GetComponent<T>(entity);
			component = value;
		}

		// 生存確認
		bool IsAlive(Entity entity) const
		{
			return entityManager.IsAlive(entity);
		}

		// --- System Management ---

		// システムを登録する
		// world.AddSystem<MovementSystem>();
		template <typename T, typename... Args>
		T* AddSystem(Args&&... args)
		{
			// メモリ確保して所有権を持つ
			auto sys = std::make_unique<T>(std::forward<Args>(args)...);
			T* rawPtr = sys.get();

			// 初期化
			rawPtr->Initialize(this);

			systems.push_back(std::move(sys));
			return rawPtr;
		}

		// 全システムの更新
		void UpdateSystems()
		{
			for (auto& sys : systems)
			{
				if (sys->IsEnabled())
				{
					sys->OnUpdate();
				}
			}
		}

		// 全システムの終了処理
		void ShutdownSystem()
		{
			for (auto& sys : systems)
			{
				sys->OnDestroy();
			}
			systems.clear();
		}

	private:
		EntityManager entityManager;
		ArchetypeManager archetypeManager;

		// システムの所有権リスト
		std::vector<std::unique_ptr<System>> systems;

		// ID -> 住所 の高速検索マップ
		std::unordered_map<EntityID, EntityLocation> entityLocationMap;

		// ヘルパー: インデックスからチャンクを特定する
		Chunk* GetTargetChunk(Archetype* arch, uint32 totalIndex)
		{
			const auto& chunks = arch->GetChunks();
			for (auto it = chunks.rbegin(); it != chunks.rend(); ++it)
			{
				Chunk* chunk = *it;
				return chunk;
			}
			return nullptr;
		}

		// コンポーネントの初期化ヘルパー (可変長テンプレート展開)
		template <typename... Ts>
		void InitializeComponents(const EntityLocation& loc)
		{
			(InitializeComponent<Ts>(loc), ...);
		}

		template <typename T>
		void InitializeComponent(const EntityLocation& loc)
		{
			size_t offset = loc.PtrArchetype->GetComponentOffset(ComponentType<T>::GetID());
			uint8* ptr = loc.PtrChunk->Memory + offset;
			T* array = reinterpret_cast<T*>(ptr);

			// 配置newでコンストラクタを呼び出す
			new (&array[loc.IndexInChunk]) T();
		}

		// --- ヘルパー関数 ---
		template <typename... ComponentTypes, typename Func, size_t... Indices>
		void ProcessChunk_Impl(Chunk* chunk, uint32 count, const std::tuple<decltype(sizeof(ComponentTypes))...>& offsets, Func&& func, std::index_sequence<Indices...>)
		{
			// 各コンポーネント配列の先頭アドレスを取得
			std::tuple<ComponentTypes*...> arrays = std::make_tuple(
				reinterpret_cast<ComponentTypes*>(chunk->Memory + std::get<Indices>(offsets))...
			);

			// EntityID配列へのポインタ
			EntityID* entityIDs = reinterpret_cast<EntityID*>(chunk->Memory);

			// チャンク内ループ
			for (uint32 i = 0; i < count; ++i)
			{
				Entity entity = { entityIDs[i] };

				// ユーザー関数を実行: func(entity, compA[i], compB[i]...)
				func(entity, (std::get<Indices>(arrays)[i])...);
			}
		}

		template <typename... ComponentTypes, typename Func>
		void ProcessChunk(Chunk* chunk, uint32 count, const std::tuple<decltype(sizeof(ComponentTypes))...>& offsets, Func&& func)
		{
			ProcessChunk_Impl<ComponentTypes...>(chunk, count, offsets, func, std::index_sequence_for<ComponentTypes...>{});
		}
	};
}
