/*****************************************************************//**
 * @file	ShadowPass.h
 * @brief	シャドウ生成パス (Directional / Spot Light 兼用・Array対応版)
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Core/Shader.h"
#include "Graphics/Core/ShadowMap.h"
#include "Graphics/Resources/Mesh.h"
#include "Graphics/Renderer.h"

namespace Span
{
	class Renderer;

	/**
	 * @class	ShadowPass
	 * @brief	シャドウ生成パス
	 */
	class ShadowPass
	{
	public:
		ShadowPass() = default;
		~ShadowPass() { Shutdown(); }

		bool Initialize(ID3D12Device* device, uint32 width, uint32 height, uint32 arraySize = 1, bool isCube = false);
		void Shutdown();

		/**
		 * @brief	シャドウマップ描画の準備（レンダーターゲット切り替え・クリア）
		 */
		void BeginPass(ID3D12GraphicsCommandList* cmd);

		/**
		 * @brief	描画するスライスを指定してクリアする
		 */
		void SetRenderTarget(ID3D12GraphicsCommandList* cmd, uint32 sliceIndex = 0);

		/**
		 * @brief	影を落とすメッシュをシャドウマップに描画します。
		 */
		void DrawMesh(Renderer* renderer, ID3D12GraphicsCommandList* cmd, Mesh* mesh, const Matrix4x4& worldMatrix, const Matrix4x4& lightSpaceMatrix);

		/**
		 * @brief	シャドウマップ描画の終了（リソースステートの復帰）
		 */
		void EndPass(ID3D12GraphicsCommandList* cmd);

		ShadowMap* GetShadowMap() const { return m_shadowMap; }

	private:
		ShadowMap* m_shadowMap = nullptr;
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		Shader* m_shaderVS = nullptr;
	};
}
