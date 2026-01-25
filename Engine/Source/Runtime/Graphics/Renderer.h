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
	using Microsoft::WRL::ComPtr;

	// 定数バッファ用構造体 (GPUに送るデータ)
	struct TransformData
	{
		Matrix4x4 MVP;
		Matrix4x4 World;
	};

	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		// 初期化: Windowではなく、初期化済みのGraphicsContextを受け取る
		bool Initialize(GraphicsContext* context);
		void Shutdown();

		// フレーム開始・終了
		ID3D12GraphicsCommandList* BeginFrame();
		void EndFrame();

		// リサイズ対応
		void OnResize(uint32 width, uint32 height);

		// 描画コマンド
		void DrawMesh(Mesh* mesh, Material* material, const Matrix4x4& worldMatrix);
		void SetCamera(const Matrix4x4& view, const Matrix4x4 projection);

		// ゲッター
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

		// パイプラインステート (PSO)
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipelineState;			  // 不透明用
		ComPtr<ID3D12PipelineState> pipelineStateTransparent; // 透明用

		// シェーダー
		Shader* vs = nullptr;
		Shader* ps = nullptr;

		// 定数バッファ (Transform行列用)
		static const uint32 MAX_OBJECTS = 10000;
		static const uint32 CB_OBJ_SIZE = 256; // 256バイトアライメント
		ComPtr<ID3D12Resource> constantBuffer;
		UINT8* mappedConstantBuffer = nullptr;
		uint32 constantBufferIndex = 0;

		// カメラ行列
		Matrix4x4 viewMatrix;
		Matrix4x4 projectionMatrix;
	};
}