/*****************************************************************//**
 * @file	World.h
 * @brief	ECSワールド (シーン) の管理クラス。
 *
 * @details
 * エンティティの生成、コンポーネントの操作、システム実行の統括を行います。
 * 通常、1つのシーンにつき1つのWorldインスタンスが存在します。
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "EntityManager.h"
#include "ArchetypeManager.h"
#include "System.h"

namespace Span
{
	/**
	 * @struct	EntityLocation
	 * @brief	📍 エンティティのデータ格納場所を示すポインタ。
	 *
	 * @details
	 * エンティティIDから実際のコンポーネントデータへアクセスするための「住所」です。
	 * これにより 0(1) でのコンポーネントアクセスが可能になります。
	 */
	struct EntityLocation
	{
		Archetype* PtrArchetype;	///< 所属しているアーキタイプへのポインタ
		Chunk* PtrChunk;			///< データが格納されているチャンクへのポインタ
		uint32 IndexInChunk;		///< チャンク内でのインデックス (0 ~ ChunkCapacity)
	};

	/**
	 * @struct	World
	 * @brief	🌏 ECSの管理マネージャー。全てのEntityとSystemを保持します。
	 *
	 * @details
	 * WorldはECSパターンのコンテナであり、以下の役割を持ちます。
	 * ### 🏗️ アーキテクチャ (Architecture)
	 * | Module               | Description |
	 * | :---                 | :--- |
	 * | **EntityManager**    | Entity IDの発行、生存管理 (生死判定) を行います。 |
	 * | **ArchetypeManager** | コンポーネントの組み合わせ (型) 毎にデータを分類・管理します。 |
	 * | **System Manager**   | 登録されたシステムの更新順序を制御し、実行します。 |
	 *
	 * ### 🔄 メモリフロー (Memory Flow)
	 * 1. `CreateEntity` でエンティティを生成
	 * 2. コンポーネント構成に基づいて適切な `Archetype` が選択される。
	 * 3. その中の `Chunk` (16KBブロック) にメモリが確保され、データが配置される。
	 */
	class World
	{
	public:
		World() = default;
		~World() = default;

		SPAN_NON_COPYABLE(World);

		// 🏭 Entity Management
		// ============================================================

		/**
		 * @brief	指定されたコンポーネントを持つ新しいエンティティを作成します。
		 * @tparam	ComponentTypes 初期状態で持たせるコンポーネントのリスト
		 * @return	作成されたEntityハンドル
		 *
		 * @code	{.cpp}
		 * // TransformとMeshRendererを持つEntityを作成
		 * Entity e = world.CreateEntity<Transform, MeshRenderer>();
		 * @endcode
		 */
		template <typename... ComponentTypes>
		Entity CreateEntity()
		{
			// 1. IDを発行
			Entity entity = entityManager.CreateEntity();

			// 2. 適切なアーキタイプを取得
			Archetype* archetype = archetypeManager.GetOrCreateArchetype<ComponentTypes...>();

			// 3. アーキタイプ内のチャンクに場所を確保
			uint32 index = archetype->AllocateEntity(entity.ID);
			Chunk* chunk = archetype->GetChunks().back();

			// 4. コンポーネントの初期化
			EntityLocation loc{ archetype, chunk, index };
			entityLocationMap[entity.ID] = loc;

			InitializeComponents<ComponentTypes...>(loc);
			return entity;
		}

		/**
		 * @brief	エンティティを削除します。
		 * @param	entity 削除対象
		 * @note
		 * 削除は **Swap-back (末尾入れ替え)** 方式が行われます。
		 * 対象データを削除した後、空いた穴に配列の末尾要素を移動させることで、
		 * 常にメモリの連続性を保ちます (0(1) コスト)。
		 */
		void DestroyEntity(Entity entity)
		{
			if (!IsAlive(entity)) return;

			auto it = entityLocationMap.find(entity.ID);
			if (it == entityLocationMap.end()) return;

			EntityLocation loc = it->second;
			Chunk* chunk = loc.PtrChunk;

			// アーキタイプから削除 (Swap-back removal)
			uint32 lastIndex = chunk->Count - 1;
			EntityID lastEntityID = reinterpret_cast<EntityID*>(chunk->Memory)[lastIndex];

			if (loc.IndexInChunk != lastIndex)
			{
				// データの移動
				chunk->MoveEntityData(loc.PtrArchetype, lastIndex, loc.IndexInChunk);

				// 移動させたEntityの住所録を更新
				entityLocationMap[lastEntityID].IndexInChunk = loc.IndexInChunk;
			}

			// チャンクのカウントを減らす
			chunk->Count--;

			// マップから削除
			entityLocationMap.erase(entity.ID);

			// ID管理システムに返却
			entityManager.DestroyEntity(entity);
		}

		/**
		 * @brief	エンティティが現在有効かどうかを確認します。
		 * @param	entity 確認対象
		 * @retval	true 生存している
		 * @retval	false 既に破棄されている
		 */
		bool IsAlive(Entity entity) const
		{
			return entityManager.IsAlive(entity);
		}

		// 🧩 Component Management
		// ============================================================

		/**
		 * @brief	既存のEntityに新しいコンポーネントを追加します。
		 *
		 * @details
		 * 構造的変更 (Structural Change) が発生し、エンティティのメモリ移動が伴います。
		 *
		 * @tparam	T 追加するコンポーネントの型
		 * @param	entity 対象エンティティ
		 * @param	initialValue 初期値
		 */
		template <typename T>
		void AddComponent(Entity entity, const T& initialValue = T())
		{
			if (!IsAlive(entity)) return;
			if (HasComponent<T>(entity)) return;

			EntityLocation oldLoc = entityLocationMap[entity.ID];
			Archetype* oldArchetype = oldLoc.PtrArchetype;

			// 新しいアーキタイプ構成を作成
			std::vector<ComponentTypeID> types = oldArchetype->GetTypes();
			std::vector<size_t> sizes;
			std::vector<size_t> aligns;

			// 既存コンポーネント情報
			for (auto id : types)
			{
				sizes.push_back(oldArchetype->GetComponentSize(id));
				aligns.push_back(oldArchetype->GetComponentAlignment(id));
			}

			// 新規コンポーネント追加
			types.push_back(ComponentType<T>::GetID());
			sizes.push_back(sizeof(T));
			aligns.push_back(alignof(T));

			Archetype* newArchetype = archetypeManager.GetOrCreateArchetype(types, sizes, aligns);

			// データ移行
			MigrateEntity(entity, newArchetype);

			// 新しいコンポーネントに値を設定
			if (T* ptr = GetComponentPtr<T>(entity))
			{
				new (ptr) T(initialValue);
			}
		}

		/**
		 * @brief	エンティティから指定したコンポーネントを削除します。
		 *
		 * @details	構造体変更が発生します。
		 *
		 * @tparam	T 削除するコンポーネントの型
		 * @param	entity 対象エンティティ
		 */
		template <typename T>
		void RemoveComponent(Entity entity)
		{
			if (!IsAlive(entity)) return;
			if (!HasComponent<T>(entity)) return;

			if (T* ptr = GetComponentPtr<T>(entity))
			{
				ptr->~T();
			}

			EntityLocation oldLoc = entityLocationMap[entity.ID];
			Archetype* oldArchetype = oldLoc.PtrArchetype;

			// 新しいアーキタイプ構成を作成
			std::vector<ComponentTypeID> types;
			std::vector<size_t> sizes;
			std::vector<size_t> aligns;

			ComponentTypeID removeID = ComponentType<T>::GetID();

			for (auto id : oldArchetype->GetTypes())
			{
				if (id == removeID) continue;

				types.push_back(id);
				sizes.push_back(oldArchetype->GetComponentSize(id));
				aligns.push_back(oldArchetype->GetComponentAlignment(id));
			}

			Archetype* newArchetype = archetypeManager.GetOrCreateArchetype(types, sizes, aligns);

			// データ移行
			MigrateEntity(entity, newArchetype);
		}

		/**
		 * @brief	指定したコンポーネントを持っているか確認します。
		 */
		template <typename T>
		bool HasComponent(Entity entity)
		{
			if (!IsAlive(entity)) return false;
			auto it = entityLocationMap.find(entity.ID);
			if (it == entityLocationMap.end()) return false;

			// アーキタイプがそのコンポーネントを持っているか確認
			return it->second.PtrArchetype->HasComponent(ComponentType<T>::GetID());
		}

		/**
		 * @brief	コンポーネントへの参照を取得します。
		 *
		 * @tparam	T 取得したいコンポーネントの型
		 * @param	entity 対象エンティティ
		 * @return	コンポーネントへの参照。持っていない場合は `nullptr`。
		 */
		template <typename T>
		T& GetComponent(Entity entity)
		{
			if (!HasComponent<T>(entity))
			{
				static T dummy{};
				return dummy;
			}

			return GetComponentUnsafe<T>(entityLocationMap[entity.ID]);
		}

		/**
		 * @brief	コンポーネントへのポインタを取得します。
		 *
		 * @tparam	T 取得したいコンポーネントの型
		 * @param	entity 対象エンティティ
		 * @return	コンポーネントへのポインタ。持っていない場合は `nullptr`。
		 */
		template <typename T>
		T* GetComponentPtr(Entity entity)
		{
			if (!IsAlive(entity)) return nullptr;

			auto it = entityLocationMap.find(entity.ID);
			if (it == entityLocationMap.end()) return nullptr;

			// 持っていない場合
			if (!it->second.PtrArchetype->HasComponent(ComponentType<T>::GetID())) return nullptr;

			return &GetComponentUnsafe<T>(it->second);
		}

		/**
		 * @brief	コンポーネントの値を設定 (上書き) します。
		 * @note	エンティティがそのコンポーネントを持っていない場合、処理はスキップされます。
		 */
		template <typename T>
		void SetComponent(Entity entity, const T& value)
		{
			if (T* ptr = GetComponentPtr<T>(entity))
			{
				*ptr = value;
			}
		}

		// ⚙ System Management
		// ============================================================

		/**
		 * @brief	システムをワールドに登録します。
		 *
		 * @tparam	T Systemクラス (Systemを継承していること)
		 * @param	args システムのコンストラクタに渡す引数
		 * @return	登録されたシステムへの生ポインタ
		 */
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

		/**
		 * @brief	全システムのアクティブな `OnUpdate` を呼び出します。
		 * @details	通常、ゲームループの毎フレームで呼び出されます。
		 */
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

		/**
		 * @brief	全システムの終了処理を行い、リストをクリアします。
		 */
		void ShutdownSystem()
		{
			for (auto& sys : systems)
			{
				sys->OnDestroy();
			}
			systems.clear();
		}

		// 🔄 Query / Iteration
		// ============================================================

		/**
		 * @brief	指定したコンポーネントを持つ全てのEntityに対して関数を実行します。
		 * @details
		 * アーキタイプごとに整理されたメモリ (Chunk) をシーケンシャルにアクセスするため、
		 * 非常にキャッシュ効率が良く、高速に動作します。
		 *
		 * @tparam	ComponentTypes 要求するコンポーネントの型リスト
		 * @param	func 実行するラムダ式 `[](Entity e, ComponentType&... comps) { ... }`
		 *
		 * @code	{.cpp}
		 * // 全てのTransformを持つエンティティの位置を更新
		 * world.ForEach<Transform, Velocity>([](Entity e, Transform& t, Velocity& v)
		 * {
		 *     t.Position += v.Value * DeltaTime;
		 * });
		 * @endcode
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

	private:
		EntityManager entityManager;
		ArchetypeManager archetypeManager;

		// システムの所有権リスト
		std::vector<std::unique_ptr<System>> systems;

		// ID -> 住所 の高速検索マップ
		std::unordered_map<EntityID, EntityLocation> entityLocationMap;	///< IDからメモリ位置への高速ルックアップテーブル

		// --- Internal Helper Methods ---

		// アーキタイプ間の移動
		void MigrateEntity(Entity entity, Archetype* newArchetype)
		{
			EntityLocation oldLoc = entityLocationMap[entity.ID];
			if (oldLoc.PtrArchetype == newArchetype) return;

			// 1. 新しい場所を確保
			uint32 newIndex = newArchetype->AllocateEntity(entity.ID);
			Chunk* newChunk = newArchetype->GetChunks().back();

			// 2. 共通のコンポーネントのデータをコピー
			for (ComponentTypeID typeID : oldLoc.PtrArchetype->GetTypes())
			{
				if (newArchetype->HasComponent(typeID))
				{
					size_t size = oldLoc.PtrArchetype->GetComponentSize(typeID);
					size_t oldOffset = oldLoc.PtrArchetype->GetComponentOffset(typeID);
					size_t newOffset = newArchetype->GetComponentOffset(typeID);

					uint8* src = oldLoc.PtrChunk->Memory + oldOffset + (oldLoc.IndexInChunk * size);
					uint8* dst = newChunk->Memory + newOffset + (newIndex * size);

					memcpy(dst, src, size);
				}
			}

			// 3. 古い情報を削除 (Swap-back)
			Chunk* oldChunk = oldLoc.PtrChunk;
			uint32 lastIndex = oldChunk->Count - 1;
			EntityID lastEntityID = reinterpret_cast<EntityID*>(oldChunk->Memory)[lastIndex];

			if (oldLoc.IndexInChunk != lastIndex)
			{
				oldChunk->MoveEntityData(oldLoc.PtrArchetype, lastIndex, oldLoc.IndexInChunk);
				entityLocationMap[lastEntityID].IndexInChunk = oldLoc.IndexInChunk;
			}
			oldChunk->Count--;

			// 4. マップ更新
			EntityLocation newLoc{ newArchetype, newChunk, newIndex };
			entityLocationMap[entity.ID] = newLoc;
		}

		// ヘルパー: Locationからコンポーネント参照を解決
		template <typename T>
		T& GetComponentUnsafe(const EntityLocation& loc)
		{
			size_t offset = loc.PtrArchetype->GetComponentOffset(ComponentType<T>::GetID());
			uint8* componentArrayBase = loc.PtrChunk->Memory + offset;
			T* componentArray = reinterpret_cast<T*>(componentArrayBase);
			return componentArray[loc.IndexInChunk];
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
			T& val = GetComponentUnsafe<T>(loc);
			new (&val) T();
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

