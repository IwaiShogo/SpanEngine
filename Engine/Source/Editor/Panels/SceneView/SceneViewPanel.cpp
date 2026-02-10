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

		// 投影設定用変数
		ProjectionType projType = ProjectionType::Perspective;
		float fov = 60.0f;
		float orthoSize = 10.0f;
		float nearClip = 0.1f;
		float farClip = 1000.0f;

		// シーン内のアクティブなカメラ(EditorCamera優先)を探す
		world.ForEach<Camera, LocalToWorld>([&](Entity e, Camera& cam, LocalToWorld& ltw)
		{
			// EditorCameraコンポーネントを持っていればそれを優先採用
			if (world.HasComponent<EditorCamera>(e) || cameraEntity.IsNull())
			{
				cameraEntity = e;
				cameraView = ltw.Value.Invert();

				// カメラ設定コピー
				projType = cam.Projection;
				fov = cam.Fov;
				orthoSize = cam.OrthographicSize;
				nearClip = cam.NearClip;
				farClip = cam.FarClip;
			}
		});

		if (cameraEntity.IsNull()) return;

		ImGuizmo::SetOrthographic(projType == ProjectionType::Orthographic);

		// 1. プロジェクション行列 (RH: ImGuizmo用)
		// ============================================================
		float aspect = size.x / size.y;
		if (aspect <= 0.0f) aspect = 1.0f;

		Matrix4x4 projRH;

		if (projType == ProjectionType::Perspective)
		{
			projRH.FromXM(XMMatrixPerspectiveFovRH(ToRadians(fov), aspect, nearClip, farClip));
		}
		else
		{
			// Orthographic行列
			projRH.FromXM(XMMatrixOrthographicRH(orthoSize * aspect, orthoSize, nearClip, farClip));
		}

		// 2. View行列の変換 (LH -> RH)
		// ============================================================
		Matrix4x4 viewRH = cameraView;
		viewRH.m[0][2] *= -1.0f;
		viewRH.m[1][2] *= -1.0f;
		viewRH.m[2][2] *= -1.0f;
		viewRH.m[3][2] *= -1.0f;

		// 3. Scene Gizmo (ViewManipulate)
		// ------------------------------------------------------------
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			// A. ギズモのサイズと位置の計算
			// パネルの短辺の 15% のサイズにする (最小 90px, 最大 150px)
			float minDim = std::min(m_PanelSize.x, m_PanelSize.y);
			float gizmoSize = Span::Clamp(minDim * 0.15f, 90.0f, 150.0f);

			// 右上の余白
			float padding = 15.0f;
			Vector2 center = { pos.x + size.x - (gizmoSize * 0.5f) - padding, pos.y + (gizmoSize * 0.5f) + padding };

			// 軸の長さ (円の半径)
			float radius = gizmoSize * 0.5f;

			// B. 軸の定義とソート
			struct Axis
			{
				Vector3 Direction;
				ImU32 Color;
				ImU32 HoverColor;
				std::string Label;
				float Depth;	// ソート用
			};

			// 軸データ (Right, Up, Forward, Left, Down, Back)
			// 左手系 (Z+奥) を前提
			std::vector<Axis> axes =
			{
				{ Vector3::Right,	IM_COL32(255, 60, 60, 255),	  IM_COL32(255, 120, 120, 255), "X", 0.0f }, // +X (Red)
				{ Vector3::Up,		IM_COL32(60, 255, 60, 255),	  IM_COL32(120, 255, 120, 255), "Y", 0.0f }, // +Y (Green)
				{ Vector3::Forward, IM_COL32(60, 60, 255, 255),	  IM_COL32(120, 120, 255, 255), "Z", 0.0f }, // +Z (Blue)
				{ Vector3::Left,	IM_COL32(180, 180, 180, 255), IM_COL32(220, 220, 220, 255), "", 0.0f },	 // -X (Grey)
				{ Vector3::Down,	IM_COL32(180, 180, 180, 255), IM_COL32(220, 220, 220, 255), "", 0.0f },	 // -Y (Grey)
				{ Vector3::Back,	IM_COL32(180, 180, 180, 255), IM_COL32(220, 220, 220, 255), "", 0.0f },	 // -Z (Grey)
			};

			// 各軸のスクリーン上の位置と深度を計算
			for (auto& axis : axes)
			{
				XMVECTOR vWorld = axis.Direction.ToXM();
				XMVECTOR vView = XMVector3TransformNormal(vWorld, cameraView.ToXM());
				XMFLOAT3 vRes;
				XMStoreFloat3(&vRes, vView);

				// 深度 (Z値が大きいほど奥)
				axis.Depth = vRes.z;
			}

			// 奥にある順にソート (Zが大きい順)
			std::sort(axes.begin(), axes.end(), [](const Axis& a, const Axis& b)
			{
					return a.Depth > b.Depth;
			});

			// マウス座標取得
			ImVec2 imMousePos = ImGui::GetMousePos();
			Vector2 mousePos = { imMousePos.x, imMousePos.y };

			bool anyHovered = false;

			for (const auto& axis : axes)
			{
				// スクリーン座標への投影
				XMVECTOR vWorld = axis.Direction.ToXM();
				XMVECTOR vView = XMVector3TransformNormal(vWorld, cameraView.ToXM());
				XMFLOAT3 vRes;
				XMStoreFloat3(&vRes, vView);

				// Y軸は反転
				Vector2 screenDir = { vRes.x, -vRes.y };
				Vector2 axisPos = { center.x + screenDir.x * radius, center.y + screenDir.y * radius };

				// 判定用サイズ
				float circleRadius = (axis.Label.empty()) ? 7.0f : 9.0f;	// ラベルあり(手前)

				// 距離判定
				float dist = std::sqrt(std::pow(mousePos.x - axisPos.x, 2) + std::pow(mousePos.y - axisPos.y, 2));
				bool hovered = (dist < circleRadius + 2.0f);	// 少し当たり判定を大きく
				if (hovered) anyHovered = true;

				// 線の描画 (中心から円まで)
				if (axis.Label.empty())
				{
					// 裏側の軸は細い線と小さな円
					drawList->AddLine(ImVec2(center.x, center.y), ImVec2(axisPos.x, axisPos.y), axis.Color, 2.0f);
					drawList->AddCircleFilled(ImVec2(axisPos.x, axisPos.y), circleRadius, hovered ? axis.HoverColor : axis.Color);
				}
				else
				{
					// 表側の軸は太い線と大きな円 + 文字
					drawList->AddLine(ImVec2(center.x, center.y), ImVec2(axisPos.x, axisPos.y), axis.Color, 3.0f);
					drawList->AddCircleFilled(ImVec2(axisPos.x, axisPos.y), circleRadius, hovered ? axis.HoverColor : axis.Color);

					// 文字 (X, Y, Z)
					ImVec2 textSize = ImGui::CalcTextSize(axis.Label.c_str());
					drawList->AddText(ImVec2(axisPos.x - textSize.x * 0.5f, axisPos.y - textSize.y * 0.5f), IM_COL32(0, 0, 0, 255), axis.Label.c_str());
				}

				// クリック処理
				if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					if (world.IsAlive(cameraEntity))
					{
						Transform* tr = world.GetComponentPtr<Transform>(cameraEntity);
						if (tr)
						{
							// 軸の方向から中心を見る位置へ
							float distFromOrigin = tr->Position.Length();
							if (distFromOrigin < 1.0f) distFromOrigin = 10.0f;

							// クリックされた軸の方向にある位置へ移動
							Vector3 newPos = axis.Direction * distFromOrigin;

							// 原点(0, 0, 0)を見る回転を計算
							Matrix4x4 lookAt = Matrix4x4::LookAtLH(newPos, Vector3::Zero, Vector3::Up);

							// View行列の逆がカメラのWorld行列
							Matrix4x4 camWorld = lookAt.Invert();

							Vector3 p, s;
							Quaternion r;
							if (camWorld.Decompose(p, r, s))
							{
								tr->Position = newPos;
								tr->Rotation = r;
							}
						}
					}
				}
			}

			// 中心ボタン: Perspective/Ortho 切り替え
			{
				float centerRadius = 5.0f;
				// マウス判定
				float dist = std::sqrt(std::pow(mousePos.x - center.x, 2) + std::pow(mousePos.y - center.y, 2));
				bool centerHovered = (dist < centerRadius + 2.0f);

				ImU32 centerColor = centerHovered ? IM_COL32(220, 220, 220, 255) : IM_COL32(255, 255, 255, 255);
				drawList->AddCircleFilled(ImVec2(center.x, center.y), centerRadius, centerColor);

				// クリックで切り替え
				if (centerHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					if (world.IsAlive(cameraEntity))
					{
						Camera* camComp = world.GetComponentPtr<Camera>(cameraEntity);
						if (camComp)
						{
							// トグル切り替え
							if (camComp->Projection == ProjectionType::Perspective)
								camComp->Projection = ProjectionType::Orthographic;
							else
								camComp->Projection = ProjectionType::Perspective;
						}
					}
				}

				// ツールチップ
				if (centerHovered)
				{
					ImGui::SetTooltip(projType == ProjectionType::Perspective ? "Switch to Orthographic" : "Switch to Perspective");
				}
			}
		}

		// 4. Object Manipulate
		// ------------------------------------------------------------
		Entity selectedEntity = SelectionManager::GetPrimary();
		if (!selectedEntity.IsNull() && world.IsAlive(selectedEntity) && m_GizmoType != -1)
		{
			Transform* tc = world.GetComponentPtr<Transform>(selectedEntity);
			if (tc)
			{
				// オブジェクト行列を転置して渡す
				Matrix4x4 objectMtx = Matrix4x4::TRS(tc->Position, tc->Rotation, tc->Scale);

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
					(float*)&viewRH,
					(float*)&projRH,
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
					Matrix4x4 newTransform = objectMtx;

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
