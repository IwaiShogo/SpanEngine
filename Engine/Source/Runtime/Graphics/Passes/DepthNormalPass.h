/*****************************************************************//**
 * @file	DepthNormalPass.h
 * @brief	SSAO等のポストプロセスで使用する、視点空間の法線と深度を描画するプリパス。
 * 
 * @details	
 * 16bit浮動小数点のテクスチャ(R16G16B16A16_FLOAT)に、
 * RGB:	View-Space Normal (視点空間法線)
 * A:	Linear Depth (視点からの距離)
 * を出力します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Core/Shader.h"
#include "Graphics/Core/RenderTarget.h"
#include "Graphics/Resources/Mesh.h"
#include "Graphics/Renderer.h"

namespace Span
{
	class Renderer;

	/**
	 * @class	DepthNormalPass
	 * @brief	🔍 画面全体の法線と深度をGBufferに出力するパス。
	 */
	class DepthNormalPass
	{
	public:
		DepthNormalPass() = default;
		~DepthNormalPass() { Shutdown(); }

		/**
		 * @brief	パスとGBufferの初期化。
		 */
		bool Initialize(ID3D12Device* device, uint32 width, uint32 height);
		void Shutdown();

		/**
		 * @brief	画像サイズ変更時にGBufferを作り直します。
		 */
		void Resize(ID3D12Device* device, uint32 width, uint32 height);

		/**
		 * @brief	描画の準備
		 */
		void BeginPass(ID3D12GraphicsCommandList* cmd);

		/**
		 * @brief	オブジェクトを描画し、法線と深度を記録します。
		 */
		void DrawMesh(Renderer* renderer, ID3D12GraphicsCommandList* cmd, Mesh* mesh, const Matrix4x4& worldMatrix, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

		/**
		 * @brief	描画の終了
		 */
		void EndPass(ID3D12GraphicsCommandList* cmd);

		/// @brief	生成されたGBufferを取得します。
		RenderTarget* GetGBuffer() const { return m_gBuffer; }

	private:
		RenderTarget* m_gBuffer = nullptr;
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12RootSignature> m_rootSignature;

		Shader* m_shaderVS = nullptr;
		Shader* m_shaderPS = nullptr;

		// シェーダーに渡す定数バッファの構造体
		struct DepthNormalData
		{
			Matrix4x4 MVP;
			Matrix4x4 World;
			Matrix4x4 View;
		};
	};
}
