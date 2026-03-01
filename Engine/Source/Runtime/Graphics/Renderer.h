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
#include "Core/Math/SpanMath.h"
#include "Core/GraphicsContext.h"
#include "Core/Shader.h"
#include "Core/ConstantBuffer.h"
#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Runtime/Scene/EnvironmentSettings.h"

// 前方宣言
namespace Span
{
	class RenderPassManager;
	class LightManager;
	class ShadowMap;
	class RenderTarget;
	class ComputeBuffer;
}

namespace Span
{
	/// @brief	オブジェクトごとの変換行列データ
	struct TransformData
	{
		Matrix4x4 MVP;
		Matrix4x4 World;
	};

	struct alignas(16) LightDataGPU
	{
		// 16 bytes
		Vector3 Color;
		float Intensity;

		// 16 bytes
		Vector3 Position;
		float Range;

		// 16 bytes
		Vector3 Direction;
		int Type;	// 0: Directional, 1: Point, 2: Spot

		// 16 bytes
		float InnerConeAngle;
		float OuterConeAngle;
		int CastShadows;	// 1: 影あり, 0: なし
		int ShadowIndex;	// 影のテクスチャ配列インデックス (影なしの場合は -1)

		// 64 bytes
		Matrix4x4 ShadowMatrix;
	};

	// 最大ライト数
	constexpr int MAX_LIGHTS = 4096;

	struct alignas(16) GlobalLightData
	{
		Vector3 CameraPosition = { 0.0f, 0.0f, 0.0f };
		float Exposure = 1.0f;

		// IBL用 (空の色)
		Vector3 SkyTopColor = { 0.35f, 0.5f, 0.7f };
		float AmbientIntensity = 1.0f;
		Vector3 SkyHorizonColor = { 0.7f, 0.75f, 0.8f };
		float EnvReflectionIntensity = 2.0f;
		Vector3 SkyBottomColor = { 0.2f, 0.2f, 0.2f };
		int ActiveLightCount = 0;

		int SkyMode;
		int EnableSSAO;
		uint32 ScreenWidth;
		uint32 ScreenHeight;

		// 光の視点の行列
		Matrix4x4 DirectionalLightSpaceMatrix;
	};

	/**
	 * @class	Renderer
	 * @brief	🖌 描画コマンドの発行を担うクラス。
	 *
	 * @details
	 * - **Root Signature**: シェーダーへの「入力スロット」定義。
	 * - **PSO (Pipeline State Object)**: シェーダー、ブレンド設定、深度設定などをまとめた状態オブジェクト。
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

		/// @brief	フレーム共通リソースをバインドする
		void BindGlobalResources();

		/**
		 * @brief	メッシュ描画コマンドの発行。
		 * @param	mesh 描画するメッシュ
		 * @param	material 適用マテリアル
		 * @param	worldMatrix ワールド変換行列
		 */
		void DrawMesh(Mesh* mesh, Material* material, const Matrix4x4& worldMatrix);

		/// @brief	Camera
		/// @{
		/// @brief	カメラ情報を更新します。
		void SetCamera(const Matrix4x4& view, const Matrix4x4 projection);
		void SetViewMatrix(const Matrix4x4& view) { viewMatrix = view; }
		void SetProjectionMatrix(const Matrix4x4& proj) { projectionMatrix = proj; }

		/// @brief	カメラ位置のセッター
		void SetCameraPosition(const Vector3& pos) { cameraPosition = pos; }
		/// @}
		
		bool LoadEnvironmentMap(const std::string& filepath);

		/// @brief	指定スロットにテクスチャをバインドします。Nullの場合は安全なダミーを生成します。
		void BindTexture(ID3D12GraphicsCommandList* cmd, Texture* texture, uint32 rootIndex, D3D12_SRV_DIMENSION dimension = D3D12_SRV_DIMENSION_TEXTURE2D);

		/// @brief	指定スロットにシャドウマップをバインドします。Nullの場合は安全なダミーを生成します。
		void BindShadowMap(ID3D12GraphicsCommandList* cmd, ShadowMap* shadowMap, uint32 rootIndex, D3D12_SRV_DIMENSION dimension = D3D12_SRV_DIMENSION_TEXTURE2D);

		/// @brief	指定スロットにRenderTargetのSRVをバインドします。
		void BindRenderTargetSRV(ID3D12GraphicsCommandList* cmd, RenderTarget* renderTarget, uint32 rootIndex, D3D12_SRV_DIMENSION dimension = D3D12_SRV_DIMENSION_TEXTURE2D);

		/// @brief	ComputeBufferをSRVとしてバインドします。
		void BindComputeBufferSRV(ID3D12GraphicsCommandList* cmd, ComputeBuffer* buffer, uint32 rootIndex);

		/// @brief	Compute Shader 用に SRV をバインドします。
		void BindComputeSRV(ID3D12GraphicsCommandList* cmd, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, uint32 rootIndex);

		/// @brief	Compute Shader 用に UAV をバインドします。
		void BindComputeUAV(ID3D12GraphicsCommandList* cmd, D3D12_CPU_DESCRIPTOR_HANDLE uavHandle, uint32 rootIndex);

		/// @brief	GPUの処理完了を待機する
		void WaitForGPU();

		/**
		 * @brief	動的定数バッファのメモリを確保し、データをコピーしてGPUアドレスを返します。
		 * @return	成功時はGPUアドレス。容量オーバー時は 0 を返します。
		 */
		D3D12_GPU_VIRTUAL_ADDRESS AllocateCBV(const void* data, size_t sizeInBytes);

		// 📊 Getters
		// ============================================================
		GraphicsContext* GetContext() const { return context; }
		Matrix4x4 GetViewMatrix() const { return viewMatrix; }
		Matrix4x4 GetProjectionMatrix() const { return projectionMatrix; }
		Vector3 GetCameraPosition() const { return cameraPosition; }
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList; }
		ID3D12Device* GetDevice() const { return context ? context->GetDevice() : nullptr; }
		uint32 GetFrameCount() const { return context ? context->GetFrameCount() : 2; }
		ID3D12CommandQueue* GetCommandQueue() const { return context ? context->GetCommandQueue() : nullptr; }

		RenderPassManager* GetPassManager() const { return m_passManager.get(); }
		LightManager* GetLightManager() const { return m_lightManager.get(); }

		/// @brief	生成済みの環境Cubemapを取得します。
		Texture* GetEnvironmentCubemap() const { return m_envCubemap.get(); }

		Texture* GetOpaqueCaptureTexture() const { return m_opaqueCaptureTex.get(); }

		void ResizeOpaqueCapture(uint32 width, uint32 height);

		void CaptureOpaqueBackground(ID3D12Resource* currentRenderTarget);

	private:
		// 内部初期化関数
		bool CreateRootSignature();
		bool CreatePipelineState();
		bool CreateConstantBuffer();
		bool CreateDummyDescriptors();
		D3D12_CPU_DESCRIPTOR_HANDLE GetDummyDescriptor(D3D12_SRV_DIMENSION dimension);

	private:
		GraphicsContext* context = nullptr;
		ID3D12GraphicsCommandList* commandList = nullptr;

		// Main Pass Objects
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipelineState;			  // 不透明用
		ComPtr<ID3D12PipelineState> pipelineStateTransparent; // 透明用
		Shader* vs = nullptr;
		Shader* ps = nullptr;

		// Dynamic CBV Memory Pool
		static const uint32 MAX_OBJECTS = 10000;
		static const uint32 CB_OBJ_SIZE = 256;
		ComPtr<ID3D12Resource> constantBuffer;
		UINT8* mappedConstantBuffer = nullptr;
		uint32 constantBufferIndex = 0;

		// Camera
		Matrix4x4 viewMatrix;
		Matrix4x4 projectionMatrix;
		Vector3 cameraPosition;

		// 同期用オブジェクト
		ComPtr<ID3D12Fence> m_waitFence;
		HANDLE m_waitEvent = nullptr;
		uint64_t m_waitFenceValue = 0;

		// Light Manager
		std::unique_ptr<LightManager> m_lightManager;

		// 1フレームで使う全テクスチャのカタログ
		ComPtr<ID3D12DescriptorHeap> m_frameSrvHeap;
		uint32 m_frameSrvHeapOffset = 0;
		uint32 m_srvDescriptorSize = 0;

		// RenderPassManager
		std::unique_ptr<RenderPassManager> m_passManager;

		// Environment
		std::unique_ptr<Texture> m_envCubemap;
		std::string m_currentLoadedHDRI = "";

		std::unique_ptr<Texture> m_irradianceMap;
		std::unique_ptr<Texture> m_prefilterMap;
		std::unique_ptr<Texture> m_brdfLUT;
		std::unique_ptr<Texture> m_opaqueCaptureTex;
		uint32 m_opaqueCaptureWidth = 0;
		uint32 m_opaqueCaptureHeight = 0;
		bool m_isOpaqueCaptureFirstFrame = true;

		// ダミーDiscriptor保持用のヒープ
		ComPtr<ID3D12DescriptorHeap> m_dummySrvHeap;
		uint32 m_dummyHeapOffset = 0;
	};
}
