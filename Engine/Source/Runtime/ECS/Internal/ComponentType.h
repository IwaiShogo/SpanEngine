#pragma once
#include "Core/CoreMinimal.h"
#include <string_view>

namespace Span
{
	// コンポーネントIDの型
	using ComponentTypeID = uint32;

	/**
	 * @brief	型(T)から一意のIDを取得するテンプレートクラス
	 * ユーザーが struct Position を定義すると、
	 * ComponentType<Position>::GetID() で自動的にユニークな番号が発行される。
	 */
	template <typename T>
	class ComponentType
	{
	public:
		static ComponentTypeID GetID()
		{
			// 型の名前を取得
			const char* typeName = typeid(T).name();

			// 文字列をハッシュ化してIDにする
			static const ComponentTypeID id = static_cast<ComponentTypeID>(
				std::hash<std::string_view>()(typeName)
			);

			static bool logged = false;
			if (!logged)
			{
				SPAN_LOG("[ComponentType] Name: %s -> ID: %u\n", typeName, id);
				logged = true;
			}

			return id;
		}

		// サイズやアライアメントなどのメタ情報もここで取得できるようにする
		static size_t GetSize() { return sizeof(T); }
		static size_t GetAlignment() { return alignof(T); }
		static const char* GetName() { return typeid(T).name(); }	// デバッグ用
	};
}
