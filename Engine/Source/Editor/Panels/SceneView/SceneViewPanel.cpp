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

			bool isHovered = ImGui::IsWindowHovered();

			World& world = Application::Get().GetWorld();
			world.ForEach<EditorCamera>([isHovered](Entity, EditorCamera& ec)
			{
				ec.IsFocused = isHovered;
			});

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

				// 現在のカーソル位置を利用してドロップターゲットを設定
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						OnAssetDropped(std::filesystem::path(path));
					}
					ImGui::EndDragDropTarget();
				}

				// --- Gizmo ---
				// ImGuizmoの描画範囲設定
				ImVec2 imageScreenPos = ImGui::GetItemRectMin();

				// ImGuizmoのセットアップ
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetRect(imageScreenPos.x, imageScreenPos.y, imageSize.x, imageSize.y);

				// ギズモ有効化
				ImGuizmo::Enable(true);

				// ギズモの描画
				// ============================================================
				DrawGizmo(Vector2(imageScreenPos.x, imageScreenPos.y), imageSize);

				// ツールバー
				// ============================================================
				DrawToolbarOverlay();

				// 速度変更時のオーバーレイ
				// ============================================================
				World& world = Application::Get().GetWorld();
				world.ForEach<EditorCamera>([&](Entity, EditorCamera& ec)
				{
					// 速度変化を検知
					if (m_LastMoveSpeed < 0.0f) m_LastMoveSpeed = ec.MoveSpeed;

					if (std::abs(ec.MoveSpeed - m_LastMoveSpeed) > 0.01f)
					{
						m_LastMoveSpeed = ec.MoveSpeed;
						m_SpeedDisplayTimer = 1.5f;
					}

					// タイマー更新と描画
					if (m_SpeedDisplayTimer > 0.0f)
					{
						m_SpeedDisplayTimer -= ImGui::GetIO().DeltaTime;

						// アルファフェードアウト
						float alpha = Span::Clamp(m_SpeedDisplayTimer / 0.5f, 0.0f, 1.0f);

						// 画面中央位置を計算
						ImVec2 windowCenter = ImGui::GetWindowPos();
						windowCenter.x += ImGui::GetWindowSize().x * 0.5f;
						windowCenter.y += ImGui::GetWindowSize().y * 0.5f;

						// テキスト作成
						char speedText[32];
						sprintf_s(speedText, "Speed: %.1f x", ec.MoveSpeed);

						// 背景とテキストを描画
						ImGui::SetNextWindowPos(windowCenter, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
						ImGui::SetNextWindowBgAlpha(0.6f * alpha);

						// 入力を阻止しない透過ウィンドウ
						ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;

						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
						if (ImGui::Begin("##SpeedOverlay", nullptr, flags))
						{
							ImGui::SetWindowFontScale(1.5f);
							ImGui::Text("%s", speedText);
							ImGui::SetWindowFontScale(1.0f);
						}
						ImGui::End();
						ImGui::PopStyleVar();
					}
				});
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
				projType = cam.Type;
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
				XMVECTOR vRes = XMVector3TransformNormal(axis.Direction.ToXM(), viewRH.ToXM());
				axis.Depth = XMVectorGetZ(vRes);
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
				XMVECTOR vResVec = XMVector3TransformNormal(axis.Direction.ToXM(), viewRH.ToXM());
				XMFLOAT3 vRes;
				XMStoreFloat3(&vRes, vResVec);

				// Y軸は反転
				Vector2 screenDir = { vRes.x, -vRes.y };
				Vector2 axisEnd = { center.x + screenDir.x * radius, center.y + screenDir.y * radius };

				float hitRadius = (axis.Label.empty()) ? 8.0f : 12.0f;
				float dist = std::sqrt(std::pow(mousePos.x - axisEnd.x, 2) + std::pow(mousePos.y - axisEnd.y, 2));
				bool hovered = (dist < hitRadius);
				if (hovered) anyHovered = true;

				ImU32 col = hovered ? axis.HoverColor : axis.Color;

				// --- 描画処理 ---
				if (axis.Label.empty())
				{
					// 裏側の軸 (線と丸)
					drawList->AddLine(ImVec2(center.x, center.y), ImVec2(axisEnd.x, axisEnd.y), col, 2.0f);
					drawList->AddCircleFilled(ImVec2(axisEnd.x, axisEnd.y), 6.0f, col);
				}
				else
				{
					// 表側の軸 (線とコーン)
					// 線
					drawList->AddLine(ImVec2(center.x, center.y), ImVec2(axisEnd.x, axisEnd.y), col, 3.0f);

					// コーンの描画
					// 軸方向(screenDir)に対して垂直なベクトルを計算
					Vector2 perp = { -screenDir.y, screenDir.x };
					float coneWidth = 8.0f;
					float coneLength = 14.0f;

					// コーンの底辺の中心
					Vector2 coneBase = { axisEnd.x - screenDir.x * coneLength, axisEnd.y - screenDir.y * coneLength };

					// 三角形の3点
					ImVec2 pTip = ImVec2(axisEnd.x, axisEnd.y);
					ImVec2 pLeft = ImVec2(coneBase.x + perp.x * coneWidth, coneBase.y + perp.y * coneWidth);
					ImVec2 pRight = ImVec2(coneBase.x - perp.x * coneWidth, coneBase.y - perp.y * coneWidth);

					drawList->AddTriangleFilled(pTip, pLeft, pRight, col);

					// ラベル
					ImVec2 textSize = ImGui::CalcTextSize(axis.Label.c_str());
					// 少し外側にずらす
					Vector2 labelPos = { axisEnd.x + screenDir.x * 10.0f, axisEnd.y + screenDir.y * 10.0f };
					drawList->AddText(ImVec2(labelPos.x - textSize.x * 0.5f, labelPos.y - textSize.y * 0.5f), col, axis.Label.c_str());
				}

				// --- クリック処理 ---
				if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					if (world.IsAlive(cameraEntity))
					{
						Transform* tr = world.GetComponentPtr<Transform>(cameraEntity);
						if (tr)
						{
							// 軸の方向を見るように回転する
							Vector3 lookDir = axis.Direction * -1.0f;

							// ジンバルロック対策
							Vector3 up = Vector3::Up;
							if (std::abs(Vector3::Dot(lookDir.Normalized(), Vector3::Up)) > 0.99f)
							{
								// 真上・真下を見る場合は、UpベクトルをZ軸にする
								up = Vector3::Forward;
							}

							Matrix4x4 lookMtx = Matrix4x4::LookAtLH(Vector3::Zero, lookDir, up);
							Matrix4x4 camWorldRot = lookMtx.Invert();

							// 回転のみ適用
							Vector3 p, s;
							Quaternion r;
							if (camWorldRot.Decompose(p, r, s))
							{
								tr->Rotation = r;
							}
						}
					}
				}
			}

			// --- 中心キューブ: Perspective/Ortho 切り替え ---
			{
				float boxSize = 6.0f;
				ImVec2 boxMin = ImVec2(center.x - boxSize, center.y - boxSize);
				ImVec2 boxMax = ImVec2(center.x + boxSize, center.y + boxSize);

				bool centerHovered = (mousePos.x >= boxMin.x && mousePos.x <= boxMax.x && mousePos.y >= boxMin.y && mousePos.y <= boxMax.y);
				ImU32 centerColor = centerHovered ? IM_COL32(220, 220, 220, 255) : IM_COL32(255, 255, 255, 255);

				// 立方体っぽく描画
				drawList->AddRectFilled(boxMin, boxMax, centerColor, 1.0f);

				// 枠線
				drawList->AddRect(boxMin, boxMax, IM_COL32(100, 100, 100, 255));
				// 文字表示 ("Persp" / "Iso")
				const char* modeText = (projType == ProjectionType::Perspective) ? "Persp" : "Iso";
				ImVec2 txtSz = ImGui::CalcTextSize(modeText);
				// ギズモの下に表示
				drawList->AddText(ImVec2(center.x - txtSz.x * 0.5f, center.y + radius + 15.0f), IM_COL32(180, 180, 180, 255), modeText);

				if (centerHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					if (world.IsAlive(cameraEntity))
					{
						Camera* camComp = world.GetComponentPtr<Camera>(cameraEntity);
						if (camComp)
						{
							// トグル
							camComp->Type = (camComp->Type == ProjectionType::Perspective)
								? ProjectionType::Orthographic
								: ProjectionType::Perspective;
						}
					}
				}
			}
		}

		// オブジェクト操作ギズモ
		Entity selectedEntity = SelectionManager::GetPrimaryEntity();
		if (!selectedEntity.IsNull() && world.IsAlive(selectedEntity) && m_GizmoType != -1)
		{
			Transform* tc = world.GetComponentPtr<Transform>(selectedEntity);
			if (tc)
			{
				Matrix4x4 objectMtx = Matrix4x4::TRS(tc->Position, tc->Rotation, tc->Scale);
				float snapValues[3] = { 0.5f, 0.5f, 0.5f }; // 必要に応じて設定
				ImGuizmo::SetID((int)(uint64_t)selectedEntity.ID.Index);

				ImGuizmo::Manipulate(
					(float*)&viewRH, (float*)&projRH,
					(ImGuizmo::OPERATION)m_GizmoType, (ImGuizmo::MODE)m_GizmoMode,
					(float*)&objectMtx, nullptr, m_UseSnap ? snapValues : nullptr
				);

				if (ImGuizmo::IsUsing())
				{
					Vector3 p, s;
					Quaternion r;
					if (objectMtx.Decompose(p, r, s))
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
		float titleHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;

		// ツールバーの領域定義
		float toolbarHeight = 36.0f;
		ImVec2 toolbarPos = { windowPos.x, windowPos.y + titleHeight };
		ImVec2 toolbarSize = { ImGui::GetWindowWidth(), toolbarHeight };

		// 背景描画
		ImGui::GetWindowDrawList()->AddRectFilled(
			toolbarPos,
			ImVec2(toolbarPos.x + toolbarSize.x, toolbarPos.y + toolbarSize.y),
			IM_COL32(20, 20, 20, 200)
		);
		// 下線
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(toolbarPos.x, toolbarPos.y + toolbarSize.y),
			ImVec2(toolbarPos.x + toolbarSize.x, toolbarPos.y + toolbarSize.y),
			IM_COL32(0, 0, 0, 255)
		);

		// ボタン配置開始
		ImGui::SetCursorScreenPos(ImVec2(toolbarPos.x + 8.0f, toolbarPos.y + 4.0f));

		// スタイル調整
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

		auto ToolbarButton = [&](const char* label, bool active) -> bool
		{
			if (active)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.6f, 0, 1));	// アクティブ時: オレンジ
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.1f));	// 背景薄く
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1));
			}

			bool clicked = ImGui::Button(label, ImVec2(28, 28));

			ImGui::PopStyleColor(active ? 2 : 1);
			return clicked;
		};

		// ツール群 (左寄せ)
		if (ToolbarButton("Q", m_GizmoType == -1)) m_GizmoType = -1;
		ImGui::SetItemTooltip("View Tool (Q)");
		ImGui::SameLine();

		if (ToolbarButton("W", m_GizmoType == ImGuizmo::OPERATION::TRANSLATE)) m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
		ImGui::SetItemTooltip("Move Tool (W)");
		ImGui::SameLine();

		if (ToolbarButton("E", m_GizmoType == ImGuizmo::OPERATION::ROTATE)) m_GizmoType = ImGuizmo::OPERATION::ROTATE;
		ImGui::SetItemTooltip("Rotate Tool (E)");
		ImGui::SameLine();

		if (ToolbarButton("R", m_GizmoType == ImGuizmo::OPERATION::SCALE)) m_GizmoType = ImGuizmo::OPERATION::SCALE;
		ImGui::SetItemTooltip("Scale Tool (R)");
		ImGui::SameLine();

		// --- セパレータ ---
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1), "|");
		ImGui::SameLine();

		// 座標系 (Local / World)
		if (ImGui::Button(m_GizmoMode == ImGuizmo::MODE::LOCAL ? "Local" : "Grobal", ImVec2(0, 28)))
		{
			m_GizmoMode = (m_GizmoMode == ImGuizmo::MODE::LOCAL) ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL;
		}
		ImGui::SetItemTooltip("Toggle Coordinate Space");

		// --- セパレータ ---
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1), "|");
		ImGui::SameLine();

		// スナップ設定
		if (ToolbarButton("S", m_UseSnap)) m_UseSnap = !m_UseSnap;
		ImGui::SetItemTooltip("Toggle Snap (Hold Ctrl)");

		// 右寄せグループ
		float rightWidth = 150.0f;
		float currentX = ImGui::GetCursorScreenPos().x;
		float targetX = toolbarPos.x + toolbarSize.x - rightWidth;

		if (targetX > currentX)
		{
			ImGui::SameLine();
			ImGui::SetCursorScreenPos(ImVec2(targetX, toolbarPos.y + 4.0f));
		}

		ImGui::PushItemWidth(100);
		ImGui::PopStyleColor(3);

		const char* aspectItems[] = { "Free", "16:9", "16:10", "4:3", "21:9" };
		int currentItem = (int)m_AspectRatio;
		ImGui::PushItemWidth(80);
		if (ImGui::Combo("##Aspect", &currentItem, aspectItems, IM_ARRAYSIZE(aspectItems)))
		{
			m_AspectRatio = (AspectRatioType)currentItem;
		}
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
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

	void SceneViewPanel::OnAssetDropped(const std::filesystem::path& path)
	{
		// 拡張子で処理を分岐
		std::string ext = path.extension().string();

		// 1. モデルの場合 -> Entity生成
		if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
		{
			auto meshPtr = AssetManager::Get().GetMesh(path.string());

			auto materialPtr = AssetManager::Get().GetDefaultMaterial();

			if (meshPtr)
			{
				auto entity = EntityBuilder(&Application::Get().GetWorld(), path.stem().string().c_str())
					.Add(Transform{ {0,0,0}, {0,0,0,1}, {1,1,1} })
					.Add(LocalToWorld{})
					.Add(MeshFilter(meshPtr.get()))
					.Add(MeshRenderer{ materialPtr.get()})
					.Build();

				SelectionManager::Select(entity);

				SPAN_LOG("Spawned Entity from: %s", path.string().c_str());
			}
		}
		// 2. シーンの場合 -> シーンロード (将来実装)
		else if (ext == ".span")
		{
			SPAN_WARN("Scene loading from Drag&Drop is not implemented yet.");
			// SceneManager::LoadScene(path.string());
		}
		// 3. テクスチャの場合 -> 選択中のオブジェクトのマテリアルに適用 or UI Entityとして配置 (将来実装)
		else if (ext == ".png" || ext == ".jpg")
		{
			SPAN_LOG("Texture dropped. (Material assignment TODO)");
		}
	}
}
