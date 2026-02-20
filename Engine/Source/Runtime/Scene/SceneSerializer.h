/*****************************************************************//**
 * @file	SceneSerializer.h
 * @brief	ECSワールド(シーン)のデータをJSON形式で保存・読み込みするクラス。
 * 
 * @details	
 * nlhmann/jsonを利用し、現在Worldに存在する全てのEntityと、
 * それらが持つComponentのデータを .span ファイルとして直列化(Serialize)します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

#include "Runtime/ECS/Kernel/World.h"

namespace Span
{
	/**
	 * @class	SceneSerializer
	 * @brief	シーンデータのシリアライズ・デシリアライズを担当するクラス。
	 */
	class SceneSerializer
	{
	public:
		/**
		 * @brief	コンストラクタ。対象となるWorldの参照を受け取ります。
		 * @param	world シリアライズ対象のWorld
		 */
		SceneSerializer(World& world);

		/**
		 * @brief	現在のWorldの状態を指定したファイルパス(.span)に保存します。
		 * @param	filepath 保存先のファイルパス
		 * @return	保存に成功した場合はtrue
		 */
		bool Serialize(const std::filesystem::path& filepath);

		/**
		 * @brief	指定したファイルパス(.span)からWorldの状態を復元します。
		 * @param	filepath 読み込み元のファイルパス
		 * @return	読み込みに成功した場合はtrue
		 */
		bool Deserialize(const std::filesystem::path& filepath);

	private:
		World& m_World;
	};
}
