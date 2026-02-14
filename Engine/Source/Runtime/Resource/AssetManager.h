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

#include <SpanEngine.h>
#include "Runtime/Graphics/Resources/Texture.h"

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
		ID3D12Device* m_Device = nullptr;
		ID3D12CommandQueue* m_CommandQueue = nullptr;

		// Texture Cache
		std::map<std::string, std::shared_ptr<Texture>> m_TextureCache;
		// Mesh Cache
		std::map<std::string, std::shared_ptr<Mesh>> m_MeshCache;

		// デフォルトリソースのキャッシュ
		std::shared_ptr<Shader> m_DefaultVS;	// Vertex Shader
		std::shared_ptr<Shader> m_DefaultPS;	// Pixel Shader
		std::shared_ptr<Material> m_DefaultMaterial;

		// Thread safety
		std::mutex m_Mutex;
	};
}
