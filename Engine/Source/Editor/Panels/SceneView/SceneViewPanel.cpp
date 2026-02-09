#include "SceneViewPanel.h"

#include <SpanEngine.h>
#include <SpanEditor.h>

#include <imgui.h>
#include <ImGuizmo.h>

namespace Span
{
	static AutoRegisterPanel<SceneViewPanel> _reg("Scene");

	SceneViewPanel::SceneViewPanel()
		: EditorPanel("Scene View")
	{
		// 初期設定: 移動モード, ローカル座標
		m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
		m_GizmoMode = ImGuizmo::MODE::LOCAL;
	}

	void SceneViewPanel::OnImGuiRender()
	{
		// 余白無しで画像を表示
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		bool isOpenTmp = isOpen;
		if (ImGui::Begin(title.c_str(), &isOpenTmp, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			isOpen = isOpenTmp;

			// 利用可能なコンテンツ領域を取得
			ImVec2 avail = ImGui::GetContentRegionAvail();

			// サイズが極端に小さい場合は更新しない
			if (avail.x > 1.0f && avail.y > 1.0f)
			{
				m_PanelSize = { avail.x, avail.y };

				// アスペクト比に基づいて、実際に画像を描画する位置とサイズを計算
				Vector2 imagePos, imageSize;
				CalculateImageArea(m_PanelSize, imagePos, imageSize);

				// 整数化
				imagePos.x = std::floor(imagePos.x);
				imagePos.y = std::floor(imagePos.y);
				imageSize.x = std::floor(imageSize.x);
				imageSize.y = std::floor(imageSize.y);

				// 次のフレームのリサイズ用にサイズを保存
				m_TargetResolution = imageSize;

				// Applicationに即座に通知
				Application::Get().SetSceneViewSize((uint32)imageSize.x, (uint32)imageSize.y);

				// カーソル位置を中心に移動
				ImVec2 cursorPos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(ImVec2(cursorPos.x + imagePos.x, cursorPos.y + imagePos.y));

				// テクスチャ描画
				if (textureHandle.ptr != 0)
				{
					// ImGui::Image は現在のカーソル位置から描画する
					ImGui::Image((ImTextureID)textureHandle.ptr, ImVec2(imageSize.x, imageSize.y), ImVec2(0, 0), ImVec2(1, 1));
				}
				else
				{
					// テクスチャが無い場合は黒塗りで場所を確保
					ImVec2 screenPos = ImGui::GetCursorScreenPos();
					ImGui::GetWindowDrawList()->AddRectFilled(
						screenPos,
						ImVec2(screenPos.x + imageSize.x, screenPos.y + imageSize.y),
						IM_COL32(0, 0, 0, 255)
					);
					ImGui::Dummy(ImVec2(imageSize.x, imageSize.y));
				}

				// --- Gizmo ---
				// ImGuizmoの描画範囲設定
				ImVec2 imageScreenPos = ImGui::GetItemRectMin();

				// ImGuizmoのセットアップ
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetRect(imageScreenPos.x, imageScreenPos.y, imageSize.x, imageSize.y);

				// ギズモ有効化
				ImGuizmo::Enable(true);

				DrawGizmo(Vector2(imageScreenPos.x, imageScreenPos.y), imageSize);
				DrawToolbarOverlay();
			}
		}
		ImGui::End();

		ImGui::PopStyleVar();
	}

	void SceneViewPanel::DrawGizmo(const Vector2& pos, const Vector2& size)
	{
		// サイズが小さすぎる場合はギズモを描画しない
		if (size.x < 1.0f || size.y < 1.0f) return;

		World& world = Application::Get().GetWorld();

		// カメラ情報の取得
		Entity cameraEntity = Entity::Null;
		Matrix4x4 cameraView = Matrix4x4::Identity();
		Matrix4x4 cameraProj = Matrix4x4::Identity();
		Transform* cameraTransform = nullptr;

		// シーン内のアクティブなカメラ(EditorCamera優先)を探す
		world.ForEach<Camera, LocalToWorld>([&](Entity e, Camera& cam, LocalToWorld& ltw)
		{
			// EditorCameraコンポーネントを持っていればそれを優先採用
			if (world.HasComponent<EditorCamera>(e) || cameraEntity.IsNull())
			{
				cameraEntity = e;
				cameraView = ltw.Value.Invert();
				cameraTransform = world.GetComponentPtr<Transform>(e);

				// アスペクト比を現在のビューポートに合わせる
				float aspect = size.x / size.y;
				if (aspect <= 0.0f) aspect = 1.0f;

				cameraProj = Matrix4x4::PerspectiveFovLH(ToRadians(cam.Fov), aspect, cam.NearClip, cam.FarClip);
			}
		});

		if (cameraEntity.IsNull()) return;

		ImGuizmo::SetOrthographic(false);

		// 行列転置用バッファ
		Matrix4x4 viewMtx = cameraView.Transpose();
		Matrix4x4 projMtx = cameraProj.Transpose();

		// 1. Object Manipulate
		// ------------------------------------------------------------
		Entity selectedEntity = SelectionManager::GetPrimary();
		if (!selectedEntity.IsNull() && world.IsAlive(selectedEntity) && m_GizmoType != -1)
		{
			Transform* tc = world.GetComponentPtr<Transform>(selectedEntity);
			if (tc)
			{
				// 現在のTransform行列
				Matrix4x4 transformMatrix = Matrix4x4::TRS(tc->Position, tc->Rotation, tc->Scale);

				// オブジェクト行列も転置して渡す
				Matrix4x4 objectMtx = transformMatrix.Transpose();

				// スナップ設定
				float snapValue = 0.5f;
				if (m_GizmoType == ImGuizmo::OPERATION::ROTATE) snapValue = m_SnapValueRotate;
				else if (m_GizmoType == ImGuizmo::OPERATION::SCALE) snapValue = m_SnapValueScale;
				else snapValue = m_SnapValueMove;

				float snapValues[3] = { snapValue, snapValue, snapValue };

				// IDを設定して競合を防ぐ
				ImGuizmo::SetID((int)(uint64_t)selectedEntity.ID.Index);

				// ギズモ操作実行
				ImGuizmo::Manipulate(
					(float*)&viewMtx,
					(float*)&projMtx,
					(ImGuizmo::OPERATION)m_GizmoType,
					(ImGuizmo::MODE)m_GizmoMode,
					(float*)&objectMtx,
					nullptr,
					m_UseSnap ? snapValues : nullptr
				);

				// 操作された場合、値を書き戻す
				if (ImGuizmo::IsUsing())
				{
					// 結果を転置して戻す
					Matrix4x4 newTransform = objectMtx.Transpose();

					Vector3 p, s;
					Quaternion r;
					if (newTransform.Decompose(p, r, s))
					{
						tc->Position = p;
						tc->Rotation = r;
						tc->Scale = s;
					}
				}
			}
		}

		// 2. Scene Gizmo (ViewManipulate)
		// ------------------------------------------------------------
		{
			float viewSize = 128.0f;
			ImVec2 viewPos = ImVec2(pos.x + size.x - viewSize, pos.y);

			ImGuizmo::ViewManipulate(
				(float*)&viewMtx,
				8.0f,
				viewPos,
				ImVec2(viewSize, viewSize),
				0x10101010
			);

			// 転置されたView行列を元に戻す
			Matrix4x4 newView = viewMtx.Transpose();
			Matrix4x4 newCamWorld = newView.Invert();

			if (cameraTransform)
			{
				Vector3 p, s;
				Quaternion r;
				if (newCamWorld.Decompose(p, r, s))
				{
					// 回転のみ適用
					cameraTransform->Rotation = r;
				}
			}
		}

		// ショートカットキー処理
		if (ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && !Input::GetKey(Key::MouseRight))
		{
			if (Input::GetKeyDown(Key::Q)) m_GizmoType = -1;	// なし (選択モード)
			if (Input::GetKeyDown(Key::W)) m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			if (Input::GetKeyDown(Key::E)) m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			if (Input::GetKeyDown(Key::R)) m_GizmoType = ImGuizmo::OPERATION::SCALE;
		}
	}

	void SceneViewPanel::DrawToolbarOverlay()
	{
		ImVec2 windowPos = ImGui::GetWindowPos();
		// タイトルバーの高さを考慮
		float titleBarHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
		ImVec2 overlayPos = { windowPos.x + 10.0f, windowPos.y + titleBarHeight + 5.0f };

		ImGui::SetNextWindowPos(overlayPos);
		ImGui::SetNextWindowBgAlpha(0.35f);

		// ツールバー用のウィンドウスタイル
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

		if (ImGui::Begin("##SceneToolbar", nullptr, window_flags))
		{
			// ボタンの色設定ヘルパー
			auto ToolbarButton = [&](const char* label, bool active) -> bool
			{
				if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0.6f, 0, 1));
				bool clicked = ImGui::Button(label, ImVec2(24, 24));
				if (active) ImGui::PopStyleColor();
				return clicked;
			};

			// Q: Select (None)
			if (ToolbarButton("Q", m_GizmoType == -1)) m_GizmoType = -1;
			ImGui::SameLine();
			// W: Translate
			if (ToolbarButton("W", m_GizmoType == ImGuizmo::OPERATION::TRANSLATE)) m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			ImGui::SameLine();
			// E: Rotate
			if (ToolbarButton("E", m_GizmoType == ImGuizmo::OPERATION::ROTATE)) m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			ImGui::SameLine();
			// R: Scale
			if (ToolbarButton("R", m_GizmoType == ImGuizmo::OPERATION::SCALE)) m_GizmoType = ImGuizmo::OPERATION::SCALE;

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			// 座標系 (Local / World)
			if (ImGui::Button(m_GizmoMode == ImGuizmo::MODE::LOCAL ? "Local" : "World"))
			{
				m_GizmoMode = (m_GizmoMode == ImGuizmo::MODE::LOCAL) ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL;
			}
			ImGui::SetItemTooltip("Toggle Local/World Space");

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			// スナップ
			// アイコンの代わりにテキストで簡易表示
			if (ToolbarButton("S", m_UseSnap)) m_UseSnap = !m_UseSnap;
			ImGui::SetItemTooltip("Toggle Snap (Hold Ctrl)");

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			// アスペクト比コンボボックス
			const char* aspectItems[] = { "Free", "16:9", "16:10", "4:3", "21:9" };
			int currentItem = (int)m_AspectRatio;
			ImGui::PushItemWidth(80);
			if (ImGui::Combo("##Aspect", &currentItem, aspectItems, IM_ARRAYSIZE(aspectItems)))
			{
				m_AspectRatio = (AspectRatioType)currentItem;
			}
			ImGui::PopItemWidth();

			ImGui::End();
		}
	}

	void SceneViewPanel::CalculateImageArea(const Vector2& avail, Vector2& outPos, Vector2& outSize)
	{
		// アスペクト比の倍率計算 (W / H)
		float targetAspect = 0.0f;
		switch (m_AspectRatio)
		{
		case AspectRatioType::Ratio_16_9:	targetAspect = 16.0f / 9.0f;	break;
		case AspectRatioType::Ratio_16_10:	targetAspect = 16.0f / 10.0f;	break;
		case AspectRatioType::Ratio_4_3:	targetAspect = 4.0f / 3.0f;		break;
		case AspectRatioType::Ratio_21_9:	targetAspect = 21.0f / 9.0f;	break;
		case AspectRatioType::Free:	default:
			outPos = { 0.0f, 0.0f };
			outSize = avail;
			return;
		}

		float currentAspect = avail.x / avail.y;

		if (currentAspect > targetAspect)
		{
			// 横に長い -> 左右に黒帯 (Pillarbox)
			outSize.y = avail.y;
			outSize.x = outSize.y * targetAspect;
			outPos.y = 0.0f;
			outPos.x = (avail.x - outSize.x) * 0.5f;
		}
		else
		{
			// 縦に長い -> 上下に黒帯 (Letterbox)
			outSize.x = avail.x;
			outSize.y = outSize.x / targetAspect;
			outPos.x = 0.0f;
			outPos.y = (avail.y - outSize.y) * 0.5f;
		}
	}
}
