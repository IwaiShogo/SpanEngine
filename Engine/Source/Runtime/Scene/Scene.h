/*****************************************************************//**
 * @file	Scene.h
 * @brief	ゲームシーン全体を管理するコンテナクラス。
 *
 * @details
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
	 * @struct	EditorCameraState
	 * @brief	エディタ作業中のカメラ状態を保持する構造体
	 */
	struct EditorCameraState
	{
		float Position[3] = { 0.0f, 5.0f, -10.0f };
		float Pitch = 15.0f;
		float Yaw = 0.0f;
	};

	/**
	 * @class	Scene
	 * @brief	ECSワールドと、それに紐づくメタデータを統合管理します。
	 */
	class Scene
	{
	public:
		Scene(const std::string& name = "Untitled") : Name(name) {}
		~Scene() = default;

		std::string Name;
		World ECSWorld;

		// --- Scene Metadata ---
		uint64_t MainCameraGUID = 0;
		EditorCameraState EditorCamera;
	};
}
