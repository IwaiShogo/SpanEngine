/*****************************************************************//**
 * @file	ShadowPass.h
 * @brief	Directional Light からのシャドウマップ生成パス。
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
	 * @brief	Directional Light からのシャドウマップ生成パス。
	 */
	class ShadowPass
	{
	public:
		ShadowPass() = default;
		~ShadowPass() { Shutdown(); }

		bool Initialize(ID3D12Device* device);
		void Shutdown();

		/**
		 * @brief	シャドウマップ描画の準備（レンダーターゲット切り替え・クリア）
		 */
		void BeginPass(ID3D12GraphicsCommandList* cmd);

		/**
		 * @brief	影を落とすメッシュをシャドウマップに描画します。
		 */
		void DrawMesh(Renderer* renderer, ID3D12GraphicsCommandList* cmd, Mesh* mesh, const Matrix4x4& worldMatrix);

		/**
		 * @brief	シャドウマップ描画の終了（リソースステートの復帰）
		 */
		void EndPass(ID3D12GraphicsCommandList* cmd);

		void SetLightSpaceMatrix(const Matrix4x4& lsm) { m_lightSpaceMatrix = lsm; }
		Matrix4x4 GetLightSpaceMatrix() const { return m_lightSpaceMatrix; }
		ShadowMap* GetShadowMap() const { return m_shadowMap; }

	private:
		ShadowMap* m_shadowMap = nullptr;
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		Shader* m_shaderVS = nullptr;
		Matrix4x4 m_lightSpaceMatrix = Matrix4x4::Identity();
	};
}
