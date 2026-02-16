/*****************************************************************//**
 * @file	ComponentRegistry.h
 * @brief	コンポーネントのメタデータ管理 (リフレクション)。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "ECS/Kernel/Entity.h"
#include <functional>
#include <vector>
#include <string>

// Forward Declaration
namespace Span { class World; }

#include "ECS/Kernel/World.h"

namespace Span
{
	// 描画用の関数ポインタ型
	using DrawComponentFunc = std::function<void(Entity, World&)>;
	// 削除用の関数ポインタ型
	using RemoveComponentFunc = std::function<void(Entity, World&)>;
	// コンポーネント追加用の関数ポインタ型
	using AddComponentFunc = std::function<void(Entity, World&)>;
	// コンポーネント確認用の関数ポインタ型
	using HasComponentFunc = std::function<bool(Entity, World&)>;

	/**
	 * @struct	ComponentMetadata
	 * @brief	🗃️ コンポーネント1つ分の型情報と操作関数。
	 */
	struct ComponentMetadata
	{
		std::string Name;
		DrawComponentFunc DrawFunc;
		RemoveComponentFunc RemoveFunc;
		AddComponentFunc AddFunc;
		HasComponentFunc HasFunc;

		int Order = 0;	///< インスペクターでの表示順
	};

	/**
	 * @class	ComponentRegistry
	 * @brief	📚 全コンポーネントのメタデータを管理する静的レジストリ。
	 * 
	 * @details
	 * `SpanReflection` マクロによって、アプリ起動時に自動的に各コンポーネントの情報が登録されます。
	 * Inspectorパネルは、このレジストリを参照してUIを構築します。
	 */
	class ComponentRegistry
	{
	public:
		/**
		 * @brief	コンポーネント型を登録します。
		 * @tparam	T コンポーネント型
		 * @param	name 表示名 ("Transform" 等)
		 * @param	onGui UI描画ロジック (ラムダ式)
		 */
		template<typename T>
		static void Register(
			const std::string& name,
			DrawComponentFunc drawFunc,
			AddComponentFunc addFunc,
			HasComponentFunc hasFunc)
		{
			ComponentMetadata meta;
			meta.Name = name;
			meta.Order = (int)GetRegistry().size();

			meta.DrawFunc = drawFunc;
			meta.AddFunc = addFunc;
			meta.HasFunc = hasFunc;

			// 削除関数
			meta.RemoveFunc = [](Entity entity, World& world)
			{
				if (world.HasComponent<T>(entity))
				{
					world.RemoveComponent<T>(entity);
				}
			};

			GetRegistry().push_back(meta);
		}

		/// @brief	登録済みの全コンポーネント情報を取得します。
		static std::vector<ComponentMetadata>& GetAll()
		{
			return GetRegistry();
		}

	private:
		// 静的変数の初期化順序問題を避けるためのアクセサ
		static std::vector<ComponentMetadata>& GetRegistry()
		{
			static std::vector<ComponentMetadata> registry;
			return registry;
		}
	};
}
