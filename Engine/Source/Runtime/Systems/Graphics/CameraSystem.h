/*****************************************************************//**
 * @file	CameraSystem.h
 * @brief	カメラ行列の更新とレンダラーへの適用。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "ECS/Kernel/System.h"
#include "Application.h"
#include "Components/Core/Transform.h"
#include "Components/Core/LocalToWorld.h"
#include "Components/Graphics/Camera.h"

namespace Span
{
	/**
	 * @class	CameraSystem
	 * @brief	📷 アクティブなカメラのView/Projection行列を計算するシステム。
	 *
	 * @details
	 * `Camera` コンポーネントを持つエンティティの `LocalToWorld` を元にビュー行列 (逆行列) を作成し、
	 * ウィンドウのアスペクト比に合わせて投影行列を作成します。
	 * 計算結果は `Renderer` に送信されます。
	 */
	class CameraSystem : public System
	{
	public:
		void OnUpdate() override
		{
			auto& renderer = Application::Get().GetRenderer();

			GetWorld()->ForEach<Camera, LocalToWorld>(
				[&](Entity, Camera& cam, LocalToWorld& ltw)
				{
					// 1. View行列
					Matrix4x4 viewMatrix = ltw.Value.Invert();

					// 2. Projection行列
					float aspectRatio = Application::Get().GetSceneViewAspectRatio();

					Matrix4x4 projMatrix = Matrix4x4::PerspectiveFovLH(
						ToRadians(cam.Fov),
						aspectRatio,
						cam.NearClip,
						cam.FarClip
					);

					// 3. レンダラーに適用
					renderer.SetCamera(viewMatrix, projMatrix);
				}
			);
		}
	};
}

