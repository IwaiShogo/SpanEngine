/*****************************************************************//**
 * @file	LightManager.h
 * @brief	シーン内のライトデータ管理と Forward+ Shading の基盤。
 * 
 * @details	
 * Renderer からライト関連の責務を分離しました。
 * 将来的に Compute Shader を用いた Light Culling (Forward+) を実行します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2026/02/27	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2026/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Core/ConstantBuffer.h"
#include "Graphics/Core/ComputeBuffer.h"
#include "Graphics/Renderer.h"

namespace Span
{
	class GraphicsContext;
	struct EnvironmentSettings;

	/**
	 * @class	LightManager
	 * @brief	💡 ライトデータの管理と Forward+ のバッファを統括するマネージャー。
	 */
#pragma warning(push)
#pragma warning(disable: 4324)
	class LightManager
	{
	public:
		LightManager() = default;
		~LightManager() { Shutdown(); }

		/**
		 * @brief	バッファの初期化
		 */
		bool Initialize(ID3D12Device* device);
		void Shutdown();

		/**
		 * @brief	画面解像度に合わせて Forward+ のグリッドサイズを更新します。
		 */
		void OnResize(ID3D12Device* device, uint32 width, uint32 height);

		/**
		 * @brief	ECSから集めたライト情報を GPU 用のバッファに書き込みます。
		 */
		void UpdateLightData(const std::vector<LightDataGPU>& lights, const EnvironmentSettings& env, const Vector3& cameraPos, bool enableSSAO, uint32 screenWidth, uint32 screenHeight);

		/// @brief	メインの定数バッファのGPUアドレスを取得
		D3D12_GPU_VIRTUAL_ADDRESS GetLightBufferAddress() const;

		ComputeBuffer* GetLightBuffer() const { return m_lightDataBuffer.get(); }

		static constexpr uint32 TILE_SIZE = 16;
		static constexpr uint32 MAX_LIGHTS_PER_TILE = 256;

		ComputeBuffer* GetLightIndexCounter() const { return m_lightIndexCounter.get(); }
		ComputeBuffer* GetLightIndexList() const { return m_lightIndexList.get(); }
		ComputeBuffer* GetLightGrid() const { return m_lightGrid.get(); }
		ComputeBuffer* GetLightDataBuffer() const { return m_lightDataBuffer.get(); }

		void ExecuteLightCulling(Renderer* renderer, ID3D12GraphicsCommandList* cmd, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, uint32 screenWidth, uint32 screenHeight, RenderTarget* gBuffer);

	private:
		bool InitializeCompute(ID3D12Device* device);

	private:
		std::unique_ptr<ConstantBuffer<GlobalLightData>> m_lightConstantBuffer;
		GlobalLightData m_currentLightData;

		// ライトの配列データそのもの
		std::unique_ptr<ComputeBuffer> m_lightDataBuffer;

		// Forward+ Shading 用のバッファ
		// ============================================================

		/// @brief	[UAV] 全タイルのライトインデックス総数をカウントする(1要素: uint)
		std::unique_ptr<ComputeBuffer> m_lightIndexCounter;

		/// @brief	[UAV/SRV] 科リングを通過したライトのインデックスを平坦に並べた配列 (タイル数 * 256 要素: uint)
		std::unique_ptr<ComputeBuffer> m_lightIndexList;

		/// @brief	[UAV/SRV] 画面の各タイルが Listの「どこから」「何個」のライトを持つか記録する (タイル数要素: uint2)
		std::unique_ptr<ComputeBuffer> m_lightGrid;

		// タイルごとのフラスタムを保存するバッファ
		std::unique_ptr<ComputeBuffer> m_frustumsBuffer;

		// Compute Shader 用のオブジェクト
		ComPtr<ID3D12RootSignature> m_computeRootSignature;
		ComPtr<ID3D12PipelineState> m_psoFrustums;
		ComPtr<ID3D12PipelineState> m_psoCulling;

		std::unique_ptr<Shader> m_shaderFrustums;
		std::unique_ptr<Shader> m_shaderCulling;

		ComPtr<ID3D12PipelineState> m_psoResetCounter;			// リセット用PSO
		std::unique_ptr<Shader> m_shaderResetCounter;			// リセット用Shader
	};
#pragma warning(pop)
}
