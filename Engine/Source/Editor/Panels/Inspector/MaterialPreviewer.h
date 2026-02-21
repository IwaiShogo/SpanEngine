/*****************************************************************//**
 * @file	MaterialPreviewer.h
 * @brief	マテリアルプレビュー用のオフスクリーンレンダリング管理。
 * 
 * @details	
 * 専用の RenderTarget とプロシージャルな球体メッシュを持ち、
 * Inspector 上にマテリアルのリアルタイムプレビューを提供します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Runtime/Graphics/Core/RenderTarget.h"
#include "Runtime/Graphics/Resources/Mesh.h"
#include "Runtime/Graphics/Resources/Material.h"
#include "Runtime/Graphics/Renderer.h"

namespace Span
{
	/**
	 * @class	MaterialPreviewer
	 * @brief	🎨 インスペクタ用のマテリアル球体プレビューア
	 */
	class MaterialPreviewer
	{
	public:
		MaterialPreviewer() = default;
		~MaterialPreviewer();

		/**
		 * @brief	プレビューアを初期化します。
		 * @param	device DX12デバイス
		 */
		bool Initialize(ID3D12Device* device);
		void Shutdown();

		/**
		 * @brief	オフスクリーンターゲットに球体を描画します。
		 * @param	cmdList コマンドリスト
		 * @param	renderer メインレンダラー
		 * @param	material 適用するマテリアル
		 */
		void Render(ID3D12GraphicsCommandList* cmdList, Renderer* renderer, Material* material);

		/**
		 * @brief	描画結果のテクスチャIDを取得します。
		 */
		void* GetTextureID() const { return m_RenderTarget.GetImGuiTextureID(); }

	private:
		void GenerateSphereMesh(ID3D12Device* device);

	private:
		RenderTarget m_RenderTarget;
		Mesh* m_SphereMesh = nullptr;
		bool m_IsInitialized = false;
	};
}
