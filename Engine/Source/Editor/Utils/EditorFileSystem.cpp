/*****************************************************************//**
 * @file	EditorFileSystem.cpp
 * @brief	EditorFileSystemの実装。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include <SpanEngine.h>
#include "EditorFileSystem.h"
#include <Windows.h>
#include <shellapi.h>

namespace Span
{
	bool EditorFileSystem::MoveFile(const std::filesystem::path& source, const std::filesystem::path& destination)
	{
		// 移動先パスの構築
		std::filesystem::path targetPath = destination / source.filename();

		if (source == targetPath) return true;

		if (std::filesystem::exists(targetPath))
		{
			SPAN_ERROR("Move failed: Destination already exists '%s'", targetPath.string().c_str());
			return false;
		}

		try
		{
			// 1. 本体移動
			std::filesystem::rename(source, targetPath);

			// 2. .meta移動
			auto srcMeta = GetMetaPath(source);
			if (std::filesystem::exists(srcMeta))
			{
				auto dstMeta = GetMetaPath(targetPath);
				std::filesystem::rename(srcMeta, dstMeta);
			}

			return true;
		}
		catch (const std::exception& e)
		{
			SPAN_ERROR("FileSystem Error: %s", e.what());
			return false;
		}
	}

	bool EditorFileSystem::DeleteFile(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path)) return false;

		try
		{
			// 1. 本体削除
			if (std::filesystem::is_directory(path))
			{
				std::filesystem::remove_all(path);	// フォルダなら中身ごと
			}
			else
			{
				std::filesystem::remove(path);
			}

			// 2. .meta削除
			auto metaPath = GetMetaPath(path);
			if (std::filesystem::exists(metaPath))
			{
				std::filesystem::remove(metaPath);
			}

			return true;
		}
		catch (const std::exception& e)
		{
			SPAN_ERROR("Delete Failed: %s", e.what());
			return false;
		}
	}

	bool EditorFileSystem::RenameFile(const std::filesystem::path& path, const std::string& newName)
	{
		std::filesystem::path targetPath = path.parent_path() / newName;

		if (path == targetPath) return true;

		if (std::filesystem::exists(targetPath))
		{
			SPAN_WARN("Rename failed: Target already exists '%s'", targetPath.string().c_str());
			return false;
		}

		try
		{
			// 1. 本体リネーム
			std::filesystem::rename(path, targetPath);

			// 2. .metaリネーム
			auto srcMeta = GetMetaPath(path);
			if (std::filesystem::exists(srcMeta))
			{
				auto dstMeta = GetMetaPath(targetPath);
				std::filesystem::rename(srcMeta, dstMeta);
			}
			return true;
		}
		catch (const std::exception& e)
		{
			SPAN_ERROR("Rename Failed: %s", e.what());
			return false;
		}
	}

	void EditorFileSystem::OpenInExplorer(const std::filesystem::path& path)
	{
		ShellExecuteA(NULL, "open", path.string().c_str(), NULL, NULL, SW_SHOWDEFAULT);
	}

	void EditorFileSystem::OpenExternal(const std::filesystem::path& path)
	{
		ShellExecuteA(NULL, "open", path.string().c_str(), NULL, NULL, SW_SHOWDEFAULT);
	}

	std::filesystem::path EditorFileSystem::GetMetaPath(const std::filesystem::path& assetPath)
	{
		return assetPath.string() + ".meta";
	}
}
