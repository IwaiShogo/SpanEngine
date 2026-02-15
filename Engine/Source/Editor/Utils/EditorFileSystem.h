/*****************************************************************//**
 * @file	EditorFileSystem.h
 * @brief	エディタ内でのファイル操作 (移動、削除、リネーム) を管理するクラス。
 *
 * @details	.metaファイルの同期処理をカプセル化します。
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

#undef MoveFile
#undef DeleteFile

namespace Span
{
	/**
	 * @brief	エディタ内でのファイル操作 (移動、削除、リネーム) を管理するクラス。
	 * @details	.metaファイルの同期処理をカプセル化します。
	 */
	class EditorFileSystem
	{
	public:
		/**
		 * @brief	ファイルまたはディレクトリを移動します (.metaも追従)。
		 * @return	成功したら true
		 */
		static bool MoveFile(const std::filesystem::path& source, const std::filesystem::path& destination);

		/**
		 * @brief	ファイルまたはディレクトリを削除します (.metaも追従)。
		 * @return	成功したら true
		 */
		static bool DeleteFile(const std::filesystem::path& path);

		/**
		 * @brief	ファイル名を変更します (.metaも追従)。
		 * @return	成功したら true
		 */
		static bool RenameFile(const std::filesystem::path& path, const std::string& newName);

		/**
		 * @brief	エクスプローラーで指定のパスを開きます。
		 */
		static void OpenInExplorer(const std::filesystem::path& path);

		/**
		 * @brief	指定されたファイルを関連付けられたアプリで開きます。
		 */
		static void OpenExternal(const std::filesystem::path& path);

	private:
		// 内部ヘルパー: .metaパスの取得
		static std::filesystem::path GetMetaPath(const std::filesystem::path& assetPath);
	};
}
