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

		return MoveFile(path, path.parent_path());

		if (std::filesystem::exists(targetPath)) return false;

		try
		{
			std::filesystem::rename(path, targetPath);

			auto srcMeta = GetMetaPath(path);
			if (std::filesystem::exists(srcMeta))
			{
				auto dstMeta = GetMetaPath(targetPath);
				std::filesystem::rename(srcMeta, dstMeta);
			}
			return true;
		}
		catch (...) { return false; }
	}

	void EditorFileSystem::OpenInExplorer(const std::filesystem::path& path)
	{
		ShellExecuteW(nullptr, L"open", L"explorer.exe", path.wstring().c_str(), nullptr, SW_SHOW);
	}

	void EditorFileSystem::OpenExternal(const std::filesystem::path& path)
	{
		ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOW);
	}

	std::filesystem::path EditorFileSystem::GetMetaPath(const std::filesystem::path& assetPath)
	{
		return assetPath.string() + ".meta";
	}
}
