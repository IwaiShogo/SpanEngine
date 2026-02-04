/*****************************************************************//**
 * @file	System.h
 * @brief	ECSシステム (ロジック) の基底クラス。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	class World;

	/**
	 * @class	System
	 * @brief	🧠 ゲームロジックを実装するための基底クラス。
	 * 
	 * @details
	 * 全てのシステムは本クラスを継承して作成します。
	 * 状態 (State) を持たず、World内のコンポーネントデータを操作することに専念してください。
	 * 
	 * ### 🔄 ライフサイクル
	 * 1. **OnCreate**: システム生成時、最初に1回だけ呼ばれます。
	 * 2. **OnUpdate**: 毎フレーム呼ばれます。メインロジックはここに記述します。
	 * 3. **OnDestroy**: ワールド破棄時やシステム削除時に呼ばれます。
	 */
	class System
	{
	public:
		virtual ~System() = default;

		/**
		 * @brief	エンジン内部で使用される初期化メソッド。ユーザーは `OnCreate` を使用してください。
		 */
		void Initialize(World* world)
		{
			m_world = world;
			OnCreate();
		}

		/**
		 * @brief	初期化時に1回だけ呼び出されます。リソースのロードや事前計算に使用します。
		 */
		virtual void OnCreate() {}

		/**
		 * @brief	毎フレーム呼び出されます。ここにゲームロジックを記述します。
		 */
		virtual void OnUpdate() {}

		/**
		 * @brief	終了時に呼び出されます。メモリ解放やクリーンアップに使用します。
		 */
		virtual void OnDestroy() {}

		/// @brief	システムが有効かどうか
		bool IsEnabled() const { return isEnabled; }

		/// @brief	システムの有効/無効を切り替えます。
		void SetEnabled(bool enabled) { isEnabled = enabled; }

	protected:
		/**
		 * @brief	所属しているワールドを取得します。
		 * @return	ワールドへのポインタ
		 */
		World* GetWorld() const { return m_world; }

	private:
		World* m_world = nullptr;
		bool isEnabled = true;
	};
}

