/*****************************************************************//**
 * @file	LayerManager.h
 * @brief	32ビットのレイヤーと衝突マトリクスを管理するクラス。
 * 
 * @details	
 * 最大32個のレイヤー (0～31) を管理します。前半 (0～7) はシステム予約、
 * 後半 (8～31) はエディタ上で自由に名前を設定できます。
 * 物理エンジンで使用するための高速なCollision Maskの取得もサポートします。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

namespace Span
{
	/**
	 * @class	LayerManager
	 * @brief	レイヤー名とレイヤー間の衝突関係を管理するシングルトン。
	 */
	class LayerManager
	{
	public:
		static LayerManager& Get()
		{
			static LayerManager instance;
			return instance;
		}

		/**
		 * @brief	指定したレイヤーの物理衝突マスク(32bit)を取得します。
		 */
		uint32_t GetCollisionMask(uint8_t layerIndex) const;

		/**
		 * @brief	レイヤー同士が衝突設定になっているか判定します。
		 */
		bool CanCollide(uint8_t layerA, uint8_t layerB) const;

		/**
		 * @brief	レイヤー間の衝突関係を設定します。
		 */
		void SetCollision(uint8_t layerA, uint8_t layerB, bool canCollide);

		/**
		 * @brief	レイヤー名を取得します。
		 */
		const std::string& GetLayerName(uint8_t layerIndex) const;

		/**
		 * @brief	ユーザーレイヤー (8～31) の名前を設定します。
		 */
		void SetLayerName(uint8_t layerIndex, const std::string& name);

		/**
		 * @brief	そのレイヤーが有効 (名前が設定されているか) 判定します。
		 */
		bool IsValidLayer(uint8_t layerIndex) const;

	private:
		LayerManager();
		~LayerManager() = default;

		std::array<std::string, 32> m_LayerNames;
		std::array<uint32_t, 32> m_CollisionMatrix;
	};

}
