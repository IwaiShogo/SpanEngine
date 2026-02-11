/*****************************************************************//**
 * @file	EditorCamera.h
 * @brief	エディタビューポート用の制御カメラタグ。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "ECS/Kernel/Entity.h"

namespace Span
{
	/**
	 * @struct	EditorCamera
	 * @brief	🎥 シーンビュー操作用のカメラを識別するためのタグコンポーネント。
	 *
	 * @details
	 * このコンポーネントを持つエンティティは、WASDキーやマウス操作によって
	 * エディタ内で自由に移動できる特殊な挙動 (CameraControllerSystem等) が適用されます。
	 * 通常のゲームプレイ用カメラと区別するために使用します。
	 *
	 */
	struct EditorCamera
	{
		// 移動設定
		float MoveSpeed = 5.0f;				///< 基本移動速度
		float SprintMultiplier = 4.0f;		///< Shiftキーでの倍率

		// 慣性・スムーズ化用
		Vector3 Velocity = Vector3::Zero;	///< 現在の速度ベクトル
		float Acceleration = 5.0f;			///< 加速度

		// ズーム設定
		float ScrollSensitivity = 2.0f;		///< ホイール感度
	};
}

