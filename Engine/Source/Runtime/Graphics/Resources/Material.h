/*****************************************************************//**
 * @file	Material.h
 * @brief	PBRマテリアルのパラメータ管理。
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
#include "Graphics/Core/ConstantBuffer.h"
#include "Texture.h"
#include "Graphics/Core/Shader.h"

namespace Span
{
	/**
	 * @struct	MaterialData
	 * @brief	📦 シェーダー(HLSL)に転送されるマテリアル定数バッファ構造体。
	 *
	 * @details
	 * HLSLの `cbuffer` は 16バイト(float4) 境界でパッキングされるため、
	 * C++側でもパディングを入れてサイズを合わせる必要があります。
	 *
	 * ### 📏 Memory Layout (16-byte alignment)
	 * | Offset | Size | Field          | Description |
	 * | :---   | :--- | :---           | :--- |
	 * | 0      | 12   | **Albedo**     | ベースカラー (RGB) |
	 * | 12     | 4    | **Roughness**  | 粗さ |
	 * | 16     | 4    | **Metallic**   | 金属度 |
	 * | 20     | 4    | **Opacity**    | 不透明度 |
	 * | 24     | 4    | **useTexture** | テクスチャ使用フラグ (boolの代わりにfloat使用) |
	 * | 28     | 4    | *Padding*      | アライメント調整用 |
	 */
	struct MaterialData
	{
		Vector3 Albedo = { 1.0f, 1.0f, 1.0f };	///< ベースカラー
		float Roughness = 0.5f;					///< 粗さ (0 = ツルツル, 1 = ザラザラ)

		float Metallic = 0.0f;					///< 金属度 (0 = 非金属, 1 = 金属)
		float Opacity = 1.0f;					///< 透明度 (1 = 不透明, 0 = 透明)
		float useTexture = 0.0f;				///< 1.0fならテクスチャ使用
		float Padding;							///< バイト数合わせ
	};

	/**
	 * @class	Material
	 * @brief	🎨 サーフェスの質感を定義するクラス。
	 *
	 * @details
	 * CPU側のパラメータ変更を検知し、描画用にGPUの定数バッファへ転送します。
	 * `Update()` を呼び出すことで `isDirty` フラグをチェックし、必要な時だけVRAM書き込みを行います。
	 */
	class Material
	{
	public:
		Material();
		~Material();

		/**
		 * @brief	マテリアル用定数バッファを初期化します。
		 * @param	device D3D12デバイス
		 */
		bool Initialize(ID3D12Device* device);

		/**
		 * @brief	終了処理
		 */
		void Shutdown();

		/**
		 * @brief	変更がある場合、データをGPUへ転送します。
		 * @note	毎フレーム描画前に呼び出してください。
		 */
		void Update();

		// 🔧 Properties
		// ============================================================

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

		/// @brief	テクスチャを使用するかどうかのフラグを設定
		void SetTexture(Texture* tex)
		{
			texture = tex;
			data.useTexture = (tex != nullptr) ? 1.0f : 0.0f;
			isDirty = true;
		}
		Texture* GetTexture() const { return texture; }

		/// @brief	マテリアルが透明か
		bool IsTransparent() const { return isTransparent; }

		/// @brief	シェーダー割り当て
		void SetShaders(Shader* vs, Shader* ps) { vertexShader = vs; pixelShader = ps; }
		Shader* GetVertexShader() const { return vertexShader; }
		Shader* GetPixelShader() const { return pixelShader; }

		/// @brief	GPUアドレス取得
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

	private:
		ConstantBuffer<MaterialData>* constantBuffer = nullptr;
		MaterialData data;
		Texture* texture = nullptr;

		// マテリアルが使用するシェーダー
		Shader* vertexShader = nullptr;
		Shader* pixelShader = nullptr;

		bool isDirty = true;	///< データに変更があったか
		bool isTransparent = false;
	};
}

