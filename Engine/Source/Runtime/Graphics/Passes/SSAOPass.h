/*****************************************************************//**
 * @file	SSAOPass.h
 * @brief	Screen Space Ambient Occlusion (SSAO) を計算パス。
 * 
 * @details	
 * DepthNormalPassで生成されたGBufferとノイズテクスチャを使用して、
 * 画面空間でのオクルージョンマップを生成します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Core/Shader.h"
#include "Graphics/Core/RenderTarget.h"
#include "Graphics/Resources/Texture.h"
#include "Graphics/Renderer.h"

namespace Span
{
	class Renderer;

	/**
	 * @class	SSAOPass
	 * @brief	🌑 SSAOを計算し、オクルージョンマップを出力するパス。
	 */
	class SSAOPass
	{
	public:
		SSAOPass() = default;
		~SSAOPass() { Shutdown(); }

		bool Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, uint32 width, uint32 height);
		void Shutdown();
		void Resize(ID3D12Device* device, uint32 width, uint32 height);

		/**
		 * @brief	SSAOの描画を実行します。
		 */
		void Execute(Renderer* renderer, ID3D12GraphicsCommandList* cmd, RenderTarget* gBuffer, const Matrix4x4& projectionMatrix);

		/// @brief	生成されたSSAOマップを取得します。
		RenderTarget* GetSSAOMap() const { return m_ssaoMap; }

	private:
		void GenerateSampleKernel();
		void GenerateNoiseTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue);

	private:
		RenderTarget* m_ssaoMap = nullptr;
		std::unique_ptr<Texture> m_noiseTexture;

		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		Shader* m_shaderVS = nullptr;
		Shader* m_shaderPS = nullptr;

		// サンプルカーネル
		std::vector<Vector4> m_ssaoKernel;

		struct SSAOData
		{
			Matrix4x4 Projection;
			Matrix4x4 InvProjection;
			Vector4 Samples[64];
			Vector2 NoiseScale;
			float Radius = 0.5f;
			float Bias = 0.025f;
		};
	};
}
