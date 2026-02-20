/*****************************************************************//**
 * @file	LayerManager.cpp
 * @brief	LayerManagerの実装。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "LayerManager.h"

namespace Span
{
	LayerManager::LayerManager()
	{
		// デフォルトレイヤーの初期化
		m_LayerNames[0] = "Default";
		m_LayerNames[1] = "TransparentFX";
		m_LayerNames[2] = "Ignore Raycast";
		m_LayerNames[3] = "";	// 未使用
		m_LayerNames[4] = "Water";
		m_LayerNames[5] = "UI";
		// 6, 7 は空き、8以降がユーザー定義

		// 初期状態では全てのレイヤーが互いに衝突する (全ビット1)
		for (int i = 0; i < 32; ++i)
		{
			m_CollisionMatrix[i] = 0xFFFFFFFF;
		}
	}

	uint32_t LayerManager::GetCollisionMask(uint8_t layerIndex) const
	{
		if (layerIndex >= 32) return 0;
		return m_CollisionMatrix[layerIndex];
	}

	bool LayerManager::CanCollide(uint8_t layerA, uint8_t layerB) const
	{
		if (layerA >= 32 || layerB >= 32) return false;
		return (m_CollisionMatrix[layerA] & (1 << layerB)) != 0;
	}

	void LayerManager::SetCollision(uint8_t layerA, uint8_t layerB, bool canCollide)
	{
		if (layerA >= 32 || layerB >= 32) return;

		if (canCollide)
		{
			m_CollisionMatrix[layerA] |= (1 << layerB);
			m_CollisionMatrix[layerB] |= (1 << layerA);
		}
		else
		{
			m_CollisionMatrix[layerA] &= ~(1 << layerB);
			m_CollisionMatrix[layerB] &= ~(1 << layerA);
		}
	}

	const std::string& LayerManager::GetLayerName(uint8_t layerIndex) const
	{
		static const std::string empty = "";
		if (layerIndex >= 32) return empty;
		return m_LayerNames[layerIndex];
	}

	void LayerManager::SetLayerName(uint8_t layerIndex, const std::string& name)
	{
		// 0～7のシステムレイヤーは変更不可とする
		if (layerIndex < 8 || layerIndex >= 32) return;
		m_LayerNames[layerIndex] = name;
	}

	bool LayerManager::IsValidLayer(uint8_t layerIndex) const
	{
		if (layerIndex >= 32) return false;
		return !m_LayerNames[layerIndex].empty();
	}
}
