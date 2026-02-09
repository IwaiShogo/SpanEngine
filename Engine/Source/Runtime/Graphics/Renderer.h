/*****************************************************************//**
 * @file	Renderer.h
 * @brief	描画パイプラインの構築と実行。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Core/GraphicsContext.h"
#include "Core/Shader.h"
#include "Core/ConstantBuffer.h"
#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Core/Math/SpanMath.h"

namespace Span
{
	/// @brief	オブジェクトごとの変換行列データ
	struct TransformData
	{
		Matrix4x4 MVP;		///< Model-View-Projection Matrix
		Matrix4x4 World;	///< World Matrix (for lighting)
	};

	/**
	 * @class	Renderer
	 * @brief	🖌 描画コマンドの発行を担うクラス。
	 * 
	 * @details
	 * - **Root Signature**: シェーダーへの「入力スロット」定義。
	 * - **PSO (Pipline State Object)**: シェーダー、ブレンド設定、深度設定などをまとめた状態オブジェクト。
	 * - **Descriptor Heap**: テクスチャや定数バッファのカタログ。
	 */
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		/// @brief	フレーム開始処理
		bool Initialize(GraphicsContext* context);
		void Shutdown();

		/// @brief	フレーム終了処理
		ID3D12GraphicsCommandList* BeginFrame();
		void EndFrame();

		/// @brief	リサイズ対応
		void OnResize(uint32 width, uint32 height);

		/**
		 * @brief	メッシュ描画コマンドの発行。
		 * @param	mesh 描画するメッシュ
		 * @param	material 適用マテリアル
		 * @param	worldMatrix ワールド変換行列
		 */
		void DrawMesh(Mesh* mesh, Material* material, const Matrix4x4& worldMatrix);

		/// @brief	カメラ情報を更新します。
		void SetCamera(const Matrix4x4& view, const Matrix4x4 projection);

		/// @brief	GPUの処理完了を待機する
		void WaitForGPU();

		// 📊 Getters
		// ============================================================

		ID3D12GraphicsCommandList* GetCommandList() const { return commandList; }
		ID3D12Device* GetDevice() const { return context ? context->GetDevice() : nullptr; }
		uint32 GetFrameCount() const { return context ? context->GetFrameCount() : 2; }
		ID3D12CommandQueue* GetCommandQueue() const { return context ? context->GetCommandQueue() : nullptr; }

	private:
		// 内部初期化関数
		bool CreateRootSignature();
		bool CreatePipelineState();
		bool CreateConstantBuffer();

	private:
		GraphicsContext* context = nullptr; // 所有権はApplicationが持つ
		ID3D12GraphicsCommandList* commandList = nullptr; // Contextからフレームごとに借りる

		// Pipeline Objects
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipelineState;			  // 不透明用
		ComPtr<ID3D12PipelineState> pipelineStateTransparent; // 透明用

		// Shaders
		Shader* vs = nullptr;
		Shader* ps = nullptr;

		// Per-Object Constant Buffer (Dynamic)
		static const uint32 MAX_OBJECTS = 10000;
		static const uint32 CB_OBJ_SIZE = 256; // 256バイトアライメント
		ComPtr<ID3D12Resource> constantBuffer;
		UINT8* mappedConstantBuffer = nullptr;
		uint32 constantBufferIndex = 0;

		// Camera
		Matrix4x4 viewMatrix;
		Matrix4x4 projectionMatrix;

		// 同期用オブジェクト
		ComPtr<ID3D12Fence> m_waitFence;
		HANDLE m_waitEvent = nullptr;
		uint64_t m_waitFenceValue = 0;
	};
}
