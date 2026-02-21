/*****************************************************************//**
 * @file	AssetManager.h
 * @brief	エンジン全域のアセットライフサイクルを管理するシングルトン。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

#include "Runtime/Graphics/Resources/Texture.h"
#include "Runtime/Graphics/Resources/Mesh.h"
#include "Runtime/Graphics/Resources/Material.h"
#include "Runtime/Graphics/ModelLoader.h"
#include "Runtime/Resource/AssetMetadata.h"

namespace Span
{
	/**
	 * @class	AssetManager
	 * @brief	📦 アセット管理システム
	 *
	 * @details
	 * **主な機能:**
	 * - **キャッシュ機構:** 一度ロードしたアセットはメモリに保持し、再利用します。
	 * - **オンデマンドロード:** 要求された瞬間にロードを行います。
	 * - **サムネイル管理:** エディタ向けにファイルパスから直接テクスチャIDを返します。
	 */
	class AssetManager
	{
	public:
		/**
		 * @brief	シングルトンインスタンスを取得します。
		 */
		static AssetManager& Get();

		/**
		 * @brief	マネージャーを初期化します。
		 * @param	device グラフィックスデバイス
		 * @param	cmdQueue コマンドキュー (リソースアップデート用)
		 */
		void Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

		/**
		 * @brief	全てのアセットを解放します。
		 */
		void Shutdown();

		// --- Core API (GUID Base) ---

		/**
		 * @brief	GUIDでテクスチャを取得 (推奨)
		 */
		std::shared_ptr<Texture> GetTexture(AssetHandle handle);

		/**
		 * @brief	GUIDでメッシュを取得 (推奨)
		 */
		std::shared_ptr<Mesh> GetMesh(AssetHandle handle);

		/**
		 * @brief	GUIDでマテリアルを取得 (推奨)
		 */
		std::shared_ptr<Material> GetMaterial(AssetHandle handle);

		// --- Legacy / Helper API (Path Base) ---

		/**
		 * @brief	テクスチャを取得します (キャッシュにあればそれを返し、無ければロードします)。
		 * @param	path アセットのファイルパス
		 * @return	テクスチャへの共有ポインタ
		 */
		std::shared_ptr<Texture> GetTexture(const std::string& path);

		/**
		 * @brief	メッシュデータを取得します (キャッシュ対応)。
		 * @param	path モデルのファイルパス
		 * @return	読み込まれたメッシュのリストへのポインタ。
		 */
		std::shared_ptr<Mesh> GetMesh(const std::string& path);

		/**
		 * @brief	パスでマテリアルを取得 (キャッシュ対応)。
		 */
		std::shared_ptr<Material> GetMaterial(const std::string& path);

		/**
		 * @brief	デフォルト (標準) のマテリアルを取得します。
		 * シェーダーやマテリアルが存在しない場合は内部で生成します。
		 */
		std::shared_ptr<Material> GetDefaultMaterial();

		/**
		 * @brief	エディタ用: ファイルパスに対応するサムネイル (テクスチャID) を取得します。
		 * @param	path 対象のファイルパス
		 * @return	ImGuiで使用可能な TextureID。画像でない場合やロード失敗時は nullptr。
		 */
		void* GetEditorThumbnail(const std::filesystem::path& path);

	private:
		AssetManager() = default;
		~AssetManager() = default;
		SPAN_NON_COPYABLE(AssetManager);

	private:
		// Texture Cache
		std::unordered_map<AssetHandle, std::shared_ptr<Texture>> m_TextureCache;
		// Mesh Cache
		std::unordered_map<AssetHandle, std::shared_ptr<Mesh>> m_MeshCache;
		// Material Cache
		std::unordered_map<AssetHandle, std::shared_ptr<Material>> m_MaterialCache;

		// デフォルトリソースのキャッシュ
		std::shared_ptr<Shader> m_DefaultVS;	// Vertex Shader
		std::shared_ptr<Shader> m_DefaultPS;	// Pixel Shader
		std::shared_ptr<Material> m_DefaultMaterial;

		// Thread safety
		ID3D12Device* m_Device = nullptr;
		ID3D12CommandQueue* m_CommandQueue = nullptr;
		std::recursive_mutex m_Mutex;
	};
}
