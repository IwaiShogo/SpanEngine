#include "HierarchyPanel.h"

#include <SpanEngine.h>
#include <SpanEditor.h>

#include <imgui.h>

namespace Span
{
	static AutoRegisterPanel<HierarchyPanel> _reg_hierarchy("Hierarchy");

	void HierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Hierarchy", &isOpen);

		World& world = Application::Get().GetWorld();

		// ルートエンティティを探して描画
		std::vector<Entity> roots;
		world.ForEach<Relationship>([&](Entity entity, Relationship& rel)
		{
			if (rel.Parent.IsNull())
			{
				roots.push_back(entity);
			}
		});

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		// パディング調整
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

		// ルート要素を描画
		for (Entity root : roots)
		{
			// EditorCameraを持っているエンティティは表示しない
			if (world.HasComponent<EditorCamera>(root))
			{
				continue;
			}

			Relationship& rel = world.GetComponent<Relationship>(root);
			if (rel.PrevSibling.IsNull())
			{
				Entity current = root;
				while (!current.IsNull())
				{
					DrawEntityNode(current);
					current = world.GetComponent<Relationship>(current).NextSibling;
				}
			}
		}

		// スタイルを戻す
		ImGui::PopStyleVar(2);

		// 空白部分でのコンテキストメニュー
		if (ImGui::BeginPopupContextWindow("HierarchyEmptyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			DrawEmptySpaceContextMenu();
			ImGui::EndPopup();
		}

		// ドラッグ&ドロップの受け皿
		if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->Rect(), ImGui::GetID("Hierarchy")))
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY_MOVE"))
			{
				Entity draggedEntity = *(const Entity*)payload->Data;
				RelationshipSystem::SetParent(&world, draggedEntity, Entity::Null);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

	void HierarchyPanel::DrawEntityNode(Entity entity)
	{
		World& world = Application::Get().GetWorld();

		// 削除済みならスキップ
		if (!world.IsAlive(entity)) return;

		Relationship& rel = world.GetComponent<Relationship>(entity);

		// IDスコープを開始
		ImGui::PushID((int)entity.ID.Index);

		// --- 1. Active Checkbox ---
		bool isActive = true;
		if (Active* a = world.GetComponentPtr<Active>(entity))
		{
			isActive = a->IsActive;
		}
		else
		{
			// 持っていなければ追加
			world.AddComponent<Active>(entity);
		}

		// 左端のマージン調整
		ImGui::Dummy(ImVec2(0, 0));
		ImGui::SameLine();

		// チェックボックスを描画
		if (ImGui::Checkbox("##Active", &isActive))
		{
			if (Active* a = world.GetComponentPtr<Active>(entity)) a->IsActive = isActive;
		}

		ImGui::SameLine();

		// --- 2. Tree Node ---

		// 表示名
		std::string name = "Entity " + std::to_string(entity.ID.Index);
		if (Name* n = world.GetComponentPtr<Name>(entity))
		{
			if (strlen(n->Value.c_str()) > 0) name = n->Value;
		}

		// フラグ設定
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

		// 選択状態
		if (SelectionManager::IsSelected(entity))
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		// 子が無ければリーフ
		if (rel.FirstChild.IsNull())
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		// 非アクティブならグレーアウト
		if (!isActive)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		}

		bool opened = ImGui::TreeNodeEx("##TreeNode", flags, name.c_str());

		if (!isActive) ImGui::PopStyleColor();

		// --- 3. Interaction ---

		// クリックで選択
		if (ImGui::IsItemClicked())
		{
			SelectionManager::Select(entity);
		}

		// ドラッグ&ドロップ処理
		HandleDragDrop(entity);

		// 右クリックメニュー
		if (ImGui::BeginPopupContextItem())
		{
			DrawContextMenu(entity);
			ImGui::EndPopup();
		}

		// 子要素の描画
		if (opened)
		{
			Entity child = rel.FirstChild;
			while (!child.IsNull())
			{
				DrawEntityNode(child);
				child = world.GetComponent<Relationship>(child).NextSibling;
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void HierarchyPanel::DrawContextMenu(Entity entity)
	{
		World& world = Application::Get().GetWorld();

		if (ImGui::MenuItem("Create Empty Child"))
		{
			Entity child = EntityBuilder(&world, "GameObject").Build();
			RelationshipSystem::SetParent(&world, child, entity);
			SelectionManager::Select(child);
		}

		if (ImGui::MenuItem("Create Cube Child"))
		{
			Entity child = EntityBuilder(&world, "Cube").Build();
			RelationshipSystem::SetParent(&world, child, entity);
			SelectionManager::Select(child);
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Delete"))
		{
			// 子も消すべきだが、とりあえず対象のみ削除
			world.DestroyEntity(entity);
			if (SelectionManager::GetPrimary() == entity)
			{
				SelectionManager::Clear();
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Duplicate"))
		{
			// 未だコピー機能が無いのでログ
			SPAN_LOG("Duplicate not implemented yet.");
		}
	}

	void HierarchyPanel::DrawEmptySpaceContextMenu()
	{
		World& world = Application::Get().GetWorld();

		if (ImGui::MenuItem("Create Empty"))
		{
			Entity e = EntityBuilder(&world, "GameObject").Build();
			SelectionManager::Select(e);
		}
	}

	void HierarchyPanel::HandleDragDrop(Entity targetEntity)
	{
		// ドラッグ開始
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("HIERARCHY_ENTITY_MOVE", &targetEntity, sizeof(Entity));
			ImGui::Text(" %s ", Application::Get().GetWorld().GetComponent<Name>(targetEntity).Value);
			ImGui::EndDragDropSource();
		}

		// ドラッグドロップ受け入れ
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY_MOVE", ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
			{
				Entity draggedEntity = *(const Entity*)payload->Data;
				World& world = Application::Get().GetWorld();

				if (draggedEntity != targetEntity)
				{
					// 矩形情報の取得
					ImVec2 min = ImGui::GetItemRectMin();
					ImVec2 max = ImGui::GetItemRectMax();
					float height = max.y - min.y;
					float mouseY = ImGui::GetMousePos().y;
					float relativeY = (mouseY - min.y) / height;

					// ガイド描画用のリスト
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					ImU32 guideColor = IM_COL32(255, 165, 0, 255); // オレンジ
					float thickness = 2.0f;

					bool isDelivery = payload->IsDelivery(); // マウスを離した瞬間か？

					// --- 上部 25%: 前に挿入 ---
					if (relativeY < 0.25f)
					{
						// 上線を描画
						drawList->AddLine(min, ImVec2(max.x, min.y), guideColor, thickness);

						if (isDelivery)
						{
							RelationshipSystem::InsertBefore(&world, draggedEntity, targetEntity);
						}
					}
					// --- 下部 25%: 後ろに挿入 ---
					else if (relativeY > 0.75f)
					{
						// 下線を描画
						drawList->AddLine(ImVec2(min.x, max.y), max, guideColor, thickness);

						if (isDelivery)
						{
							Relationship& targetRel = world.GetComponent<Relationship>(targetEntity);
							Entity nextSibling = targetRel.NextSibling;
							Entity parent = targetRel.Parent;

							// 次の兄弟の前 = 自分の後ろ
							// 親情報を渡すことで、末尾(nextSiblingがNull)の場合でも正しく追加させる
							RelationshipSystem::InsertBefore(&world, draggedEntity, nextSibling, parent);
						}
					}
					// --- 中央 50%: 子にする ---
					else
					{
						// 枠線を描画
						drawList->AddRect(min, max, guideColor, 0.0f, 0, thickness);

						if (isDelivery)
						{
							RelationshipSystem::SetParent(&world, draggedEntity, targetEntity);
						}
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
}

