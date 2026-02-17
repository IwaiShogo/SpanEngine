#pragma once
#include "Core/CoreMinimal.h"
#include "AssetMetadata.h"

namespace Span
{
	/**
	 * @class	AssetRegistry
	 * @brief	全アセットの GUID と ファイルパス の対応関係を管理するデータベース。
	 */
	class AssetRegistry
	{
	public:
		static AssetRegistry& Get();

		/**
		 * @brief	指定ディレクトリ以下の全アセットをスキャンし、レジストリを構築します。
		 * @note	通常はエンジンの初期化時に呼び出します。
		 */
		void Refresh(const std::filesystem::path& rootDirectory);

		/**
		 * @brief	パスからアセット情報を登録・更新します (ファイル移動/作成時用)。
		 */
		void RegisterAsset(const std::filesystem::path& path);
		void UnregisterAsset(const std::filesystem::path& path);

		// --- Lookups ---
		bool Contains(AssetHandle handle) const;
		bool Contains(const std::filesystem::path& path) const;

		const std::filesystem::path& GetPath(AssetHandle handle) const;
		AssetHandle GetHandle(const std::filesystem::path& path) const;

		// デバッグ用: 全アセットリスト
		const std::unordered_map<AssetHandle, std::filesystem::path>& GetEntries() const { return m_Assets; }

	private:
		// GUID -> Path (メインDB)
		std::unordered_map<AssetHandle, std::filesystem::path> m_Assets;

		// Path -> GUID (逆引き用キャッシュ)
		std::unordered_map<std::string, AssetHandle> m_PathToHandle;

		// 無効な場合のフォールバック
		std::filesystem::path m_EmptyPath;
	};
}
