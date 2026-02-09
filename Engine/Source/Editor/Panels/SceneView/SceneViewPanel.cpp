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
		m_GizmoType = ImGuizmo::MODE::LOCAL;
	}

	void SceneViewPanel::OnImGuiRender()
	{
		// 余白無しで画像を表示
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		bool isOpenTmp = isOpen;
		if (ImGui::Begin(title.c_str(), &isOpenTmp, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			isOpen = isOpenTmp;

			// --- 1. ビューポート解像度管理 ---
			ImVec2 avail = ImGui::GetContentRegionAvail();

			// ドッキング中などでサイズが極端に小さい場合は処理しない
			if (avail.x > 1.0f && avail.y > 1.0f)
			{
				m_PanelSize = { avail.x, avail.y };

				// アスペクト比に基づいて、実際に画像を描画する位置とサイズを計算 (レターボックス処理)
				Vector2 imagePos, imageSize;
				CalculateImageArea(m_PanelSize, imagePos, imageSize);

				// 次のフレームでApplicationがリサイズするべき解像度を保存
				m_TargetResolution = imageSize;

				// 計算されたサイズをApplicationに通知し、カメラのアスペクト比を同期させる
				Application::Get().SetSceneViewSize((uint32)imageSize.x, (uint32)imageSize.y);

				// --- 画像の描画 ---
				// カーソル位置を計算した開始位置に移動 (これで中央寄せされる)
				ImVec2 cursorPos = ImGui::GetCursorPos(); // 現在のカーソル（左上）
				ImGui::SetCursorPos(ImVec2(cursorPos.x + imagePos.x, cursorPos.y + imagePos.y));

				if (textureHandle.ptr != 0)
				{
					// ImGui::Image は現在のカーソル位置から描画する
					ImGui::Image((ImTextureID)textureHandle.ptr, ImVec2(imageSize.x, imageSize.y));
				}

				// --- ギズモの描画 ---
				// 画像のスクリーン座標（モニター左上からの絶対座標）を取得
				// これは ImGui::Image の直前に SetCursorPos した場所に対応する
				ImVec2 imageScreenPos = ImGui::GetItemRectMin();

				// ImGuizmoの描画範囲を「画像の描画範囲」に完全に一致させる
				// これでマウス判定のズレ（触れない問題）が直ります
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetRect(imageScreenPos.x, imageScreenPos.y, imageSize.x, imageSize.y);

				DrawGizmo(Vector2(imageScreenPos.x, imageScreenPos.y), imageSize);
				DrawToolbarOverlay();
			}
		}
		ImGui::End();

		ImGui::PopStyleVar();
	}

	void SceneViewPanel::DrawGizmo(const Vector2& pos, const Vector2& size)
	{
		World& world = Application::Get().GetWorld();

		// カメラ情報の取得
		Entity cameraEntity = Entity::Null;
		Matrix4x4 cameraView = Matrix4x4::Identity();
		Matrix4x4 cameraProj = Matrix4x4::Identity();

		// シーン内のアクティブなカメラ(EditorCamera優先)を探す
		world.ForEach<Camera, LocalToWorld>([&](Entity e, Camera& cam, LocalToWorld& ltw)
		{
			// EditorCameraコンポーネントを持っていればそれを優先採用
			if (world.HasComponent<EditorCamera>(e) || cameraEntity.IsNull())
			{
				cameraEntity = e;
				cameraView = ltw.Value.Invert();

				// アスペクト比を現在のビューポートに合わせる
				float aspect = size.x / size.y;
				if (aspect <= 0.0f) aspect = 1.0f;

				cameraProj = Matrix4x4::PerspectiveFovLH(ToRadians(cam.Fov), aspect, cam.NearClip, cam.FarClip);
			}
		});

		if (cameraEntity.IsNull()) return;

		ImGuizmo::SetOrthographic(false);

		// --- View Cube (シーンギズモ) ---
		// 右上に表示
		{
			Matrix4x4 viewManipulateMatrix = cameraView;
			float viewSize = 128.0f;

			ImGuizmo::ViewManipulate(
				(float*)&viewManipulateMatrix,
				5.0f,
				ImVec2(pos.x + size.x - viewSize, pos.y),
				ImVec2(viewSize, viewSize),
				0x10101010
			);

			// ViewManipulateで操作された場合、カメラの行列を更新する必要がある
		}

		// --- 選択オブジェクトの操作ギズモ ---
		Entity selectedEntity = SelectionManager::GetPrimary();
		if (!selectedEntity.IsNull() && world.IsAlive(selectedEntity) && m_GizmoType != -1)
		{
			Transform* tc = world.GetComponentPtr<Transform>(selectedEntity);
			if (tc)
			{
				// グリッド描画
				if (m_GizmoType == ImGuizmo::OPERATION::TRANSLATE)
				{
					// ImGuizmo::DrawGrid((float*)&cameraView, (float*)&cameraProj, (float*)&Matrix4x4::Identity(), 100.0f);
				}

				// 現在のTransform行列
				Matrix4x4 transformMatrix = Matrix4x4::TRS(tc->Position, tc->Rotation, tc->Scale);

				// スナップ設定
				float snapValue = 0.5f;
				if (m_GizmoType == ImGuizmo::OPERATION::ROTATE) snapValue = m_SnapValueRotate;
				else if (m_GizmoType == ImGuizmo::OPERATION::SCALE) snapValue = m_SnapValueScale;
				else snapValue = m_SnapValueMove;

				float snapValues[3] = { snapValue, snapValue, snapValue };

				// ギズモ操作実行
				ImGuizmo::Manipulate(
					(float*)&cameraView,
					(float*)&cameraProj,
					(ImGuizmo::OPERATION)m_GizmoType,
					(ImGuizmo::MODE)m_GizmoMode,
					(float*)&transformMatrix,
					nullptr,
					m_UseSnap ? snapValues : nullptr
				);

				// 操作された場合、値を書き戻す
				if (ImGuizmo::IsUsing())
				{
					Vector3 translation, rotation, scale;
					ImGuizmo::DecomposeMatrixToComponents((float*)&transformMatrix, &translation.x, &rotation.x, &scale.x);

					tc->Position = translation;
					tc->Rotation = Quaternion::FromEuler(ToRadians(rotation.x), ToRadians(rotation.y), ToRadians(rotation.z));
					tc->Scale = scale;
				}
			}
		}

		// --- ショートカットキー処理 ---
		// シーンビューにフォーカスがある時のみ
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
		ImVec2 overlayPos = { windowPos.x + 10.0f, windowPos.y + titleBarHeight + 10.0f };

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
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Local/World Space");

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			// スナップ
			// アイコンの代わりにテキストで簡易表示
			if (ToolbarButton("S", m_UseSnap)) m_UseSnap = !m_UseSnap;
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Snap (Hold Ctrl)");

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
