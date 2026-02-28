/*****************************************************************//**
 * @file	ComponentType.h
 * @brief	コンポーネント型システム (Type to Mapping)。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include <string_view>

namespace Span
{
	/// @brief	コンポーネントを識別するための一意なID型 (32bit整数)
	using ComponentTypeID = uint32;

	/**
	 * @brief	全てのコンポーネント型で共有されるIDジェネレータ
	 */
	class ComponentTypeCounter
	{
	public:
		static ComponentTypeID GetNextID()
		{
			static ComponentTypeID counter = 0;
			return counter++;
		}
	};

	/**
	 * @class	ComponentType
	 * @brief	🏷️ C++の型(T)をランタイムIDに変換する静的ヘルパークラス。
	 * 
	 * @details
	 * ユーザーが新しい構造体 (コンポーネント) を定義した際、手動でIDを割り当てる必要はありません。
	 * このクラスが型名からハッシュ値を計算し、自動的に一意のIDを発行します。
	 * 
	 * ### 🔄 ID生成フロー
	 * 1. `typeid(T).name()` で型名文字列を取得 (例: "struct Span::Transform")
	 * 2. `std::hash` で文字列を 32bit 整数にハッシュ化
	 * 3. 静的ローカル変数としてキャッシュし、次回以降は計算無しで返す
	 * 
	 * @tparam	T 対象となるコンポーネント構造体
	 */
	template <typename T>
	class ComponentType
	{
	public:
		/**
		 * @brief	型 `T` に対応する一意のIDを取得します。
		 * @return	ハッシュ化されたコンポーネントID
		 * @note	プログラム実行中、常に同じ値が返ることが保証されます。
		 */
		static ComponentTypeID GetID()
		{
			// 初回呼び出し時にのみ一意の連番IDを取得
			static const ComponentTypeID id = ComponentTypeCounter::GetNextID();
			return id;
		}

		/// @brief	コンポーネントのメモリサイズ (バイト)
		static size_t GetSize() { return sizeof(T); }

		/// @brief	コンポーネントのメモリアライメント要件
		static size_t GetAlignment() { return alignof(T); }

		/**
		 * @brief	デバッグ用の型名取得
		 * @return	コンパイラ依存の型名文字列 (MSVCの場合は可読性が高い)
		 */
		static const char* GetName() { return typeid(T).name(); }
	};
}

