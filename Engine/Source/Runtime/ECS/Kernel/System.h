#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	class World;

	class System
	{
	public:
		virtual ~System() = default;

		// エンジンがシステムを登録するときに呼ぶ
		void Initialize(World* world)
		{
			m_world = world;
			OnCreate();
		}

		// 初期化時に1回呼ばれる
		virtual void OnCreate() {}

		// 毎フレーム呼ばれる
		virtual void OnUpdate() {}

		// 破棄時に呼ばれる
		virtual void OnDestroy() {}

		bool IsEnabled() const { return isEnabled; }

	protected:
		// システム内からいつでもWorldにアクセスできる
		World* GetWorld() const { return m_world; }

	private:
		World* m_world = nullptr;
		bool isEnabled = true;
	};
}
