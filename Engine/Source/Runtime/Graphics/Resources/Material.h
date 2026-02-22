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
#include "Graphics/Core/Shader.h"
#include "Graphics/Resources/Texture.h"
#include "Resource/AssetMetadata.h"

namespace Span
{
	/// @brief	マテリアルの描画モード
	enum class BlendMode { Opaque = 0, Transparent, Cutout };

	/// @brief	ポリゴンのカリングモード
	enum class CullMode { Back = 0, Front, None };

	/**
	 * @struct	MaterialData
	 * @brief	📦 シェーダー(HLSL)に転送されるマテリアル定数バッファ (16バイト協会アライメント厳守)
	 */
	struct alignas(16) MaterialData
	{
		Vector4 AlbedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };	///< ベースカラーと不透明度
		Vector3 EmissiveColor = { 0.0f, 0.0f, 0.0f };		///< 発光カラー
		float Roughness = 0.5f;								///< 表面の粗さ

		float Metallic = 0.0f;								///< 金属度
		float AO = 1.0f;									///< 環境遮蔽(アンビエントオクルーション)強度
		float Cutoff = 0.5f;								///< Cutoutモード時のアルファ閾値
		float Padding1;										///< アライメント用

		Vector2 Tiling = { 1.0f, 1.0f };					///< UVタイリング
		Vector2 Offset = { 0.0f, 0.0f };					///< UVオフセット

		// Texture Use Flags
		int HasAlbedoMap = 0;
		int HasNormalMap = 0;
		int HasMetallicMap = 0;
		int HasRoughnessMap = 0;

		int HasAOMap = 0;
		int HasEmissiveMap = 0;
		int Padding2;
		int Padding3;
	};

	/**
	 * @class	Material
	 * @brief	🎨 PRBマテリアルを管理し、.matアセットとして機能するクラス
	 */
	class Material
	{
	public:
		Material();
		~Material();

		/**
		 * @brief	GPUリソースの初期化
		 */
		bool Initialize(ID3D12Device* device);

		/**
		 * @brief	GPUリソースの解放
		 */
		void Shutdown();

		/**
		 * @brief	変更がある場合、データをGPUへ転送します。
		 * @note	毎フレーム描画前に呼び出してください。
		 */
		void Update();

		/**
		 * @brief	GPU仮想アドレスの取得
		 */
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

		// 💾 Asset Serialization
		// ============================================================

		/**
		 * @brief	マテリアルを .mat ファイルとして保存します。
		 * @param	filepath 保存先パス
		 * @return	成功すれば true
		 */
		bool Serialize(const std::filesystem::path& filepath);

		/**
		 * @brief	.mat ファイルからマテリアルを読み込みます。
		 * @param	filepath 読み込み元パス
		 * @return	成功すれば true
		 */
		bool Deserialize(const std::filesystem::path& filepath);

		// 🔧 Properties
		// ============================================================

		std::string Name = "New Material";

		BlendMode GetBlendMode() const { return m_BlendMode; }
		void SetBlendMode(BlendMode mode) { m_BlendMode = mode; }

		CullMode GetCullMode() const { return m_CullMode; }
		void SetCullMode(CullMode mode) { m_CullMode = mode; }

		// Raw Data Access (Inspector用)
		MaterialData& GetData() { m_IsDirty = true; return m_Data; }

		// 🖼️ Texture Slots
		// ============================================================

		void SetAlbedoMap(Texture* tex)		{ m_AlbedoMap = tex;	m_Data.HasAlbedoMap = (tex != nullptr);		m_IsDirty = true; }
		void SetNormalMap(Texture* tex)		{ m_NormalMap = tex;	m_Data.HasNormalMap = (tex != nullptr);		m_IsDirty = true; }
		void SetMetallicMap(Texture* tex)	{ m_MetallicMap = tex;	m_Data.HasMetallicMap = (tex != nullptr);	m_IsDirty = true; }
		void SetRoughnessMap(Texture* tex)	{ m_RoughnessMap = tex;	m_Data.HasRoughnessMap = (tex != nullptr);	m_IsDirty = true; }
		void SetAOMap(Texture* tex)			{ m_AOMap = tex;		m_Data.HasAOMap = (tex != nullptr);			m_IsDirty = true; }
		void SetEmissiveMap(Texture* tex)	{ m_EmissiveMap = tex;	m_Data.HasEmissiveMap = (tex != nullptr);	m_IsDirty = true; }

		Texture* GetAlbedoMap() const { return m_AlbedoMap; }
		Texture* GetNormalMap() const { return m_NormalMap; }
		Texture* GetMetallicMap() const { return m_MetallicMap; }
		Texture* GetRoughnessMap() const { return m_RoughnessMap; }
		Texture* GetAOMap() const { return m_AOMap; }
		Texture* GetEmissiveMap() const { return m_EmissiveMap; }

		// シリアライズ用のテクスチャGUID保持用
		uint64_t AlbedoMapGUID = 0;
		uint64_t NormalMapGUID = 0;
		uint64_t MetallicMapGUID = 0;
		uint64_t RoughnessMapGUID = 0;
		uint64_t AOMapGUID = 0;
		uint64_t EmissiveMapGUID = 0;

		// Shader
		void SetShaders(Shader* vs, Shader* ps) { m_VertexShader = vs; m_PixelShader = ps; }
		Shader* GetVertexShader() const { return m_VertexShader; }
		Shader* GetPixelShader() const { return m_PixelShader; }

		AssetHandle Handle = 0;

		Texture* m_AlbedoMap = nullptr;
		Texture* m_NormalMap = nullptr;
		Texture* m_MetallicMap = nullptr;
		Texture* m_RoughnessMap = nullptr;
		Texture* m_AOMap = nullptr;
		Texture* m_EmissiveMap = nullptr;

	private:
		MaterialData m_Data;
		ConstantBuffer<MaterialData>* m_ConstantBuffer = nullptr;
		bool m_IsDirty = true;

		BlendMode m_BlendMode = BlendMode::Opaque;
		CullMode m_CullMode = CullMode::Back;

		Shader* m_VertexShader = nullptr;
		Shader* m_PixelShader = nullptr;
	};
}

