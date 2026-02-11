/*****************************************************************//**
 * @file	SceneViewPanel.h
 * @brief	レンダリング結果と操作ギズモを表示するパネル
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/Math/SpanMath.h"
#include "Editor/Panels/EditorPanel.h"

namespace Span
{
	/// @brief	シーンビューのアスペクト比設定
	enum class AspectRatioType
	{
		Free = 0,		///< ウィンドウサイズに合わせて自由変形
		Ratio_16_9,		///< 16:9  (標準的なワイド画面)
		Ratio_16_10,	///< 16:10
		Ratio_4_3,		///< 4:3 (レトロ)
		Ratio_21_9		///< 21:9 (ウルトラワイド)
	};

	/**
	 * @class	SceneViewPanel
	 * @brief	🎥 ゲームの描画結果を表示し、オブジェクト操作(Gizmo)を提供するクラス。
	 *
	 * @details
	 * `Renderer` が描画したテクスチャ(SRV)を受け取り、ImGuiのウィンドウ内に画像として表示します。
	 * その上に `ImGuizmo` を重ねることで、選択中のオブジェクトを直接操作できるようにしています。
	 *
	 * ### 🖱 操作
	 * - **右クリック**: カメラ視点移動 (WASD)
	 * - **W / E / R**: 移動 / 回転 / 拡大縮小 モード切り替え
	 * - **Q**: ローカル / ワールド 座標系切り替え
	 */
	class SceneViewPanel : public EditorPanel
	{
	public:
		SceneViewPanel();

		/**
		 * @brief	毎フレームの描画処理。
		 * 画像の表示、ツールバーのオーバーレイ、ギズモの描画を行います。
		 */
		void OnImGuiRender() override;

		/**
		 * @brief	描画するテクスチャ (レンダリング) を設定します。
		 * @param	handle GPU上のディスクリプタハンドル (SRV)
		 */
		void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle) { textureHandle = handle; }

		/**
		 * @brief	現在のパネルサイズに基づいた、レンダリング推奨解像度を取得します。
		 * @return	Resizingが必要なサイズ
		 */
		Vector2 GetTargetResolution() const { return m_TargetResolution; }

	private:
		// --- UI Helpers ---

		/// @brief	パネル上部にツールバー (ギズモ操作ボタン等) を重ねて表示します。
		void DrawToolbarOverlay();

		/// @brief	選択中のオブジェクトに対してマニピュレータ (矢印など) を描画します。
		void DrawGizmo(const Vector2& pos, const Vector2& size);

		/// @brief	アスペクト比設定に基づいて、画像を表示すべき領域を計算します。
		void CalculateImageArea(const Vector2& availableSize, Vector2& outPos, Vector2& outSize);

	private:
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = {};

		// パネル自体のサイズ
		Vector2 m_PanelSize = { 0.0f, 0.0f };

		// 実際にレンダリングする解像度
		Vector2 m_TargetResolution = { 1280.0f, 720.0f };

		// ギズモ設定
		int m_GizmoType = 0;	// 0 = None, 1 = Translate, 2 = Rotate, 3 = Scale
		int m_GizmoMode = 0;	// 座標系 (0 = Local, 1 = World)
		bool m_UseSnap = false;
		float m_SnapValueMove = 0.5f;
		float m_SnapValueRotate = 45.0f;
		float m_SnapValueScale = 0.5f;

		// アスペクト比設定
		AspectRatioType m_AspectRatio = AspectRatioType::Free;

		// 速度表示UI用
		float m_LastMoveSpeed = -1.0f;
		float m_SpeedDisplayTimer = 0.0f;
	};
}

