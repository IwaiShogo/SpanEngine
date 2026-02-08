/*****************************************************************//**
 * @file	EntityBuilder.h
 * @brief	エンティティ生成を簡略化するヘルパークラス。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "World.h"
#include "Components/Core/Name.h"
#include "Components/Core/Tag.h"
#include "Components/Core/Layer.h"
#include "Components/Core/Transform.h"
#include "Components/Core/Relationship.h"
#include "Components/Core/Active.h"

namespace Span
{
	/**
	 * @class	EntityBuilder
	 * @brief	🔨 メソッドチェーンを利用して直感的にEntityを構築するビルダークラス。
	 * 
	 * @details
	 * 標準的なコンポーネント (Name, Transform等) を自動的に付与し、
	 * fluent interface (流れるようなインターフェース) で初期値を設定できます。
	 * 
	 * ### 📝 Usage
	 * ```cpp
	 * Entity player = EntityBuilder(world, "Player");
	 *     .Add<Position>({ 0, 10, 0 })
	 *     .Add<Health>({ 100 })
	 *     .Build();
	 * ```
	 */
	class EntityBuilder
	{
	public:
		/**
		 * @brief	ビルダーを開始し、基本構成を持つEntityを作成します。
		 * @param	world 所属させるワールド
		 * @param	name エンティティ名 (Nameコンポーネントに設定されます)
		 */
		EntityBuilder(World* world, const std::string& name = "GameObject")
			: m_world(world)
		{
			// デフォルトコンポーネントをセットで作成
			m_entity = m_world->CreateEntity<Name, Tag, Layer, Transform, Relationship, Active>();

			// 初期値設定
			Name& nameComp = m_world->GetComponent<Name>(m_entity);
			nameComp.Value = name;

			// タグの初期化
			m_world->GetComponent<Tag>(m_entity).Value = "Untagged";
		}

		/**
		 * @brief	コンポーネントを追加し、初期値を設定します。
		 * @tparam	T 追加するコンポーネント型
		 * @param	componentValue 設定する初期値
		 */
		template <typename T>
		EntityBuilder& Add(const T& componentValue)
		{
			m_world->AddComponent<T>(m_entity, componentValue);
			return *this;
		}

		/**
		 * @brief	コンポーネントを追加します (初期値はデフォルトコンストラクタ)。
		 * @tparam	T 追加するコンポーネント型
		 */
		template <typename T>
		EntityBuilder& Add()
		{
			m_world->AddComponent<T>(m_entity);
			return *this;
		}

		/**
		 * @brief	特定のコンポーネントを操作
		 */
		template <typename T>
		EntityBuilder& With(std::function<void(T&)> func)
		{
			if (T* comp = m_world->GetComponentPtr<T>(m_entity))
			{
				func(*comp);
			}
			return *this;
		}

		/// @brief	構築したエンティティハンドルを取得します。
		Entity Build()
		{
			return m_entity;
		}

		/// @brief	暗黙的な変換演算子 (Entity型としてそのまま扱えるようにする)
		operator Entity() const { return m_entity; }

	private:
		World* m_world;
		Entity m_entity;
	};
}

