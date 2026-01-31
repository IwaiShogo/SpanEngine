#pragma once
#include "ECS/Kernel/Entity.h"
#include <functional>
#include <vector>
#include <string>

// Worldクラスの前方宣言だけを行う
namespace Span { class World; }

#include "ECS/Kernel/World.h"

namespace Span
{
	using DrawComponentFunc = std::function<void(Entity, World&)>;
	using RemoveComponentFunc = std::function<void(Entity, World&)>;

	struct ComponentMetadata
	{
		std::string Name;
		DrawComponentFunc DrawFunc;
		RemoveComponentFunc RemoveFunc;

		// ソート用
		int Order = 0;
	};

	class ComponentRegistry
	{
	public:
		template<typename T>
		static void Register(const std::string& name, std::function<void(T&, Entity, World&)> onGui)
		{
			ComponentMetadata meta;
			meta.Name = name;
			meta.Order = (int)GetRegistry().size(); // 登録順を初期値に

			// 描画関数
			meta.DrawFunc = [onGui](Entity entity, World& world)
			{
				if (T* component = world.GetComponentPtr<T>(entity))
				{
					onGui(*component, entity, world);
				}
			};

			// 削除関数
			meta.RemoveFunc = [](Entity entity, World& world)
			{
				world.RemoveComponent<T>(entity);
			};

			GetRegistry().push_back(meta);
		}

		static std::vector<ComponentMetadata>& GetAll() { return GetRegistry(); }

	private:
		// 静的初期化順序問題（SIOF）回避のため関数内static変数にする
		static std::vector<ComponentMetadata>& GetRegistry()
		{
			static std::vector<ComponentMetadata> registry;
			return registry;
		}
	};
}