/*****************************************************************//**
 * @file	ImGuiUI.h
 * @brief	ImGui用のカスタムウィジェットヘルパー。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/Math/SpanMath.h"
#include "Core/Log/Logger.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Span
{
	// 前方宣言
	class Texture;
	class Mesh;
	class Material;

	/**
	 * @class	ImGuiUI
	 * @brief	🎛️ エディタ独自のUIパーツを提供する静的クラス。
	 *
	 * @details
	 * UnityやUnreal Engineのような、使いやすく整ったUIコントロールを提供します。
	 */
	class ImGuiUI
	{
	public:
		/**
		 * @brief	UnityスタイルのVector3コントロールを描画します。
		 *
		 * @details
		 * 左側にラベル、右側に X, Y, Z の各フィールドを表示します。
		 * 各軸のラベル(X, Y, Z)をクリックすると、値をリセットできます。
		 * - **X**: 赤色
		 * - **Y**: 緑色
		 * - **Z**: 青色
		 *
		 * @param	label 左側に表示するラベル文字列
		 * @param	values 編集対象のVector3参照
		 * @param	resetValue 軸ラベルをクリックした際のリセット値 (Default: 0.0f)
		 * @param	columnWidth ラベルカラムの幅 (Default: 100.0f)
		 * @return	値が変更された場合 true
		 */
		static bool DrawVec3Control(const std::string& label, Vector3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

		/**
		 * @brief	コンポーネント用の折り畳みヘッダーを描画します。
		 *
		 * @details
		 * 枠線付きの `TreeNode` を描画し、右クリックメニューで「コンポーネント削除」機能を提供します。
		 *
		 * @param	name ヘッダーに表示するコンポーネント名
		 * @param[out] isRemoved 「Remove Component」が選択された場合 true がセットされる
		 * @param	defaultOpen 初期状態で開いているか
		 * @returnヘッダが開いている場合 true
		 */
		static bool DrawComponentHeader(const std::string& name, bool& isRemoved, bool defaultOpen = true);

		/**
		 * @brief	アセット選択用スロットを描画し、D&Dを受け入れる汎用ヘルパー
		 * @tparam	AssetType アセットのクラス (Texture, Mesh etc)
		 * @param	label プロパティ名
		 * @param	assetPtr アセットへのポインタ参照
		 * @param	assetName 表示するアセット名
		 * @param	extensionFilter 受け入れる拡張子 (例: ".png")
		 * @param	loaderFunc ロード関数
		 */
		template <typename AssetType, typename LoaderFunc>
		static void DrawAssetSlot(const char* label, AssetType*& assetPtr, const std::string& assetName, const std::vector<std::string>& extensions, LoaderFunc loaderFunc)
		{
			ImGui::PushID(label);

			// 2カラムレイアウト (ラベル | ボタン)
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 100.0f);
			ImGui::Text("%s", label);
			ImGui::NextColumn();

			// ボタン幅設計
			float buttonWidth = ImGui::GetContentRegionAvail().x - 30.0f;

			// アセット名の表示ボタン
			std::string displayText = assetPtr ? assetName : "None (" + std::string(typeid(AssetType).name()) + ")";

			if (assetPtr)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
			}

			ImGui::Button(displayText.c_str(), ImVec2(buttonWidth, 0.0f));

			if (assetPtr) ImGui::PopStyleColor();

			// --- ドラッグ&ドロップ受け入れ処理 ---
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* pathStr = (const wchar_t*)payload->Data;
					std::filesystem::path path(pathStr);
					std::string ext = path.extension().string();

					// 拡張子チェック
					bool isValid = false;
					for (const auto& e : extensions)
					{
						if (ext == e) { isValid = true; break; }
					}

					if (isValid)
					{
						// ロード関数を呼び出してポインタ更新
						auto sharedAsset = loaderFunc(path.string());
						if (sharedAsset)
						{
							assetPtr = sharedAsset.get();
						}
					}
					else
					{
						SPAN_WARN("Invalid asset type: %s", ext.c_str());
					}
				}
				ImGui::EndDragDropTarget();
			}

			// xボタン
			ImGui::SameLine();
			if (ImGui::Button("X", ImVec2(20, 0)))
			{
				assetPtr = nullptr;
			}

			ImGui::Columns(1);
			ImGui::PopID();
		}

		// --- 特化型ラッパー関数 ---

		static void DrawTextureSlot(const char* label, Texture*& texture);

		static void DrawMeshSlot(const char* label, Mesh*& mesh);

		static void DrawMaterialSlot(const char* label, Material*& material);
	};
}
