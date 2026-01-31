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
	class EntityBuilder
	{
	public:
		EntityBuilder(World* world, const std::string& name = "GameObject")
			: m_world(world)
		{
			// デフォルトコンポーネントをセットで作成
			m_entity = m_world->CreateEntity<Name, Tag, Layer, Transform, Relationship, Active>();

			// 初期値設定
			Name& nameComp = m_world->GetComponent<Name>(m_entity);
			strcpy_s(nameComp.Value, name.c_str());

			// タグの初期化
			m_world->GetComponent<Tag>(m_entity).Value = "Untagged";
		}

		// コンポーネント追加 & 値設定
		template <typename T>
		EntityBuilder& Add(const T& componentValue)
		{
			m_world->AddComponent<T>(m_entity, componentValue);
			return *this;
		}

		// コンポーネント追加 (デフォルト値)
		template <typename T>
		EntityBuilder& Add()
		{
			m_world->AddComponent<T>(m_entity);
			return *this;
		}

		// 特定のコンポーネントを操作
		template <typename T>
		EntityBuilder& With(std::function<void(T&)> func)
		{
			if (T* comp = m_world->GetComponentPtr<T>(m_entity))
			{
				func(*comp);
			}
			return *this;
		}

		Entity Build()
		{
			return m_entity;
		}

	private:
		World* m_world;
		Entity m_entity;
	};
}
