#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Math/SpanMath.h"
#include "Graphics/Core/ConstantBuffer.h"
#include "Texture.h"

namespace Span
{
	// シェーダーに送るデータ構造
	struct MaterialData
	{
		Vector3 Albedo = { 1.0f, 1.0f, 1.0f };	// ベースカラー
		float Roughness = 0.5f;					// 粗さ (0 = ツルツル, 1 = ザラザラ)

		float Metallic = 0.0f;					// 金属度 (0 = 非金属, 1 = 金属)
		float Opacity = 1.0f;					// 透明度 (1 = 不透明, 0 = 透明)
		float Padding[2];						// バイト数合わせ
		float useTexture = 0.0f;				// 1.0fならテクスチャ使用
	};

	class Material
	{
	public:
		Material();
		~Material();

		// 初期化
		bool Initialize(ID3D12Device* device);

		// 終了
		void Shutdown();

		// データをGPUに転送
		void Update();

		// --- プロパティ ---
		void SetAlbedo(const Vector3& color) { data.Albedo = color; isDirty = true; }
		void SetRoughness(float roughness) { data.Roughness = roughness; isDirty = true; }
		void SetMetallic(float metallic) { data.Metallic = metallic; isDirty = true; }

		void SetOpacity(float opacity)
		{
			data.Opacity = opacity;
			isDirty = true;
			// 1.0未満なら自動的に透明モードとみなすフラグ
			isTransparent = (opacity < 1.0f);
		}

		void SetTexture(Texture* tex)
		{
			texture = tex;
			data.useTexture = (tex != nullptr) ? 1.0f : 0.0f;
			isDirty = true;
		}
		Texture* GetTexture() const { return texture; }

		// マテリアルが透明か
		bool IsTransparent() const { return isTransparent; }

		// GPUアドレス取得
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

	private:
		MaterialData data;
		ConstantBuffer<MaterialData>* constantBuffer = nullptr;
		bool isDirty = true;
		bool isTransparent = false;
		Texture* texture = nullptr;
	};
}
