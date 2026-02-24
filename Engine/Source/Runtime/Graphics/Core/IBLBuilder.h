/*****************************************************************//**
 * @file	IBLBuilder.h
 * @brief	IBL(Image-Based Lighting)用テクスチャをGPU上で生成するビルダークラス。
 * 
 * @details	
 * Compute　Shader を活用し、HDRパノラマ画像から以下のPBR用リソースを事前計算します。
 * 1. Cubemap (Equirectangular to Cubemap)
 * 2. Irradiance Map (拡散反射用)
 * 3. Pre-filter Environment Map (鏡面反射用)
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2026/02/23	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2026/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Core/GraphicsContext.h"
#include "Graphics/Core/Shader.h"
#include "Graphics/Resources/Texture.h"

namespace Span
{
	/**
	 * @class	IBLBuilder
	 * @brief	🌌 パノラマHDR画像からPBRに必要なIBLテクスチャ群を生成するユーティリティ。
	 * 
	 * @details
	 * このクラスの処理は非常に重いため、ゲームのロード時やエディタでのアセットインポート時に
	 * 1度だけ (オフラインで) 実行されることを想定しています。
	 */
	class IBLBuilder
	{
	public:
		IBLBuilder() = default;
		~IBLBuilder() { Shutdown(); }

		/**
		 * @brief	IBLBuilderを初期化し、必要なCompute Shaderをロードします。
		 * @param	device DX12デバイス
		 * @return	成功すれば true
		 */
		bool Initialize(ID3D12Device* device);

		/**
		 * @brief	終了処理。ロードしたシェーダーやルートシグネチャを解放します。
		 */
		void Shutdown();

		/**
		 * @brief	パノラマHDRテクスチャをCubemapに変換します。
		 * 
		 * @param	cmd コマンドリスト
		 * @param	panoramaSRV 入力となるパノラマHDR画像のSRV
		 * @param	outCubemap 出力先のCubemapテクスチャ
		 * @param	cubemapSize 生成するCubemapの1面の解像度
		 */
		void GenerateCubemapFromPanorama(ID3D12Device* device, ID3D12GraphicsCommandList* cmd, D3D12_CPU_DESCRIPTOR_HANDLE panoramaSRV, Texture* outCubemap, uint32 cubemapSize);

		/**
		 * @brief	環境Cubemapから、Diffuse用 Irradiance Map を生成。
		 */
		void GenerateIrradianceMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmd, D3D12_CPU_DESCRIPTOR_HANDLE envCubemapSRV, Texture* outIrradianceMap, uint32 mapSize = 32);

	private:
		ComPtr<ID3D12RootSignature> m_computeRootSignature;
		ComPtr<ID3D12PipelineState> m_equirectToCubemapPSO;
		Shader* m_equirectToCubemapCS = nullptr;

		ここから
	};
}
