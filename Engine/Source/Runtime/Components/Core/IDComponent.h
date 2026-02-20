/*****************************************************************//**
 * @file	IDComponent.h
 * @brief	エンティティの一意な識別子 (UUID) を保持するコンポーネント。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

#include "Core/CoreMinimal.h"
#include "Runtime/Reflection/SpanReflection.h"

namespace Span
{
	/**
	 * @struct	IDComponent
	 * @brief	シリアライズ及びシーン間の参照解決に使用する一意のID。
	 */
	struct IDComponent
	{
		uint64 ID;

		IDComponent()
		{
			// UUIDの自動生成
			static std::random_device s_RandomDevice;
			static std::mt19937_64 s_Engine(s_RandomDevice());
			static std::uniform_int_distribution<uint64> s_UniformDistribution;

			ID = s_UniformDistribution(s_Engine);
			if (ID == 0) ID++;	// 0は無効値とする場合
		}

		IDComponent(uint64 id) : ID(id) {}

		SPAN_INSPECTOR_BEGIN(IDComponent)
			SPAN_FIELD(ID, HideInInspector())
		SPAN_INSPECTOR_END()
	};
}
