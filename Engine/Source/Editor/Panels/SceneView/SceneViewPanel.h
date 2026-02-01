#pragma once
#include "Core/Math/SpanMath.h"
#include "Editor/Panels/EditorPanel.h"

namespace Span
{
	enum class AspectRatioType
	{
		Free = 0,
		Ratio_16_9,
		Ratio_16_10,
		Ratio_4_3,
		Ratio_21_9
	};

	class SceneViewPanel : public EditorPanel
	{
	public:
		SceneViewPanel();

		void OnImGuiRender() override;

		// 表示するテクスチャ
		void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle) { textureHandle = handle; }

		// アプリケーション側がリサイズ判定に使う
		Vector2 GetTargetResolution() const { return m_TargetResolution; }

	private:
		// ツールバーを描画するヘルパー
		void DrawToolbarOverlay();
		// ギズモ操作の実装
		void DrawGizmo(const Vector2& pos, const Vector2& size);
		// アスペクト比に応じた矩形計算
		void CalculateImageArea(const Vector2& availableSize, Vector2& outPos, Vector2& outSize);

	private:
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = {};

		// パネル自体のサイズ
		Vector2 m_PanelSize = { 0.0f, 0.0f };

		// 実際にレンダリングする解像度
		Vector2 m_TargetResolution = { 0.0f, 0.0f };

		// ギズモ設定
		int m_GizmoType = 0;	// 0 = None, 1 = Translate, 2 = Rotate, 3 = Scale
		int m_GizmoMode = 0;	// 座標系 (0 = Local, 1 = World)
		bool m_UseSnap = false;
		float m_SnapValueMove = 0.5f;
		float m_SnapValueRotate = 45.0f;
		float m_SnapValueScale = 0.5f;

		// アスペクト比設定
		AspectRatioType m_AspectRatio = AspectRatioType::Free;
	};
}
