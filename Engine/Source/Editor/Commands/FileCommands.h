/*****************************************************************//**
 * @file	FileCommands.h
 * @brief	ファイル操作に関するUndo可能なコマンド群
 * 
 * @details	
 * - RenameFileCommand: ファイル名変更
 * - MoveFileCommand: ファイル移動
 * - DeleteFileCommand: 削除 (実際は .Trash への移動)
 * - CreateFileCommand: ファイル作成
 * - CreateDirectoryCommand: フォルダ削除
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Editor/Core/ICommand.h"
#include "Editor/Utils/EditorFileSystem.h"

namespace Span
{
	// Utility: ゴミ箱パスの生成
	inline std::filesystem::path GetTrashPath(const std::filesystem::path& originalPath)
	{
		auto projectRoot = originalPath.parent_path();
		while (projectRoot.has_parent_path() && projectRoot.filename() != "Assets")
		{
			projectRoot = projectRoot.parent_path();
		}
		if (projectRoot.filename() == "Assets") projectRoot = projectRoot.parent_path();

		std::filesystem::path trashDir = projectRoot / ".Trash";

		std::error_code ec;
		if (!std::filesystem::exists(transDir)) std::filesystem::create_directories(trashDir, ec);

		// 一意な名前を生成
		auto now = std::chrono::system_clock::now().time_since_epoch().count();
		std::string filename = originalPath.stem().string();
		std::string ext = originalPath.extention().string();

		std::stringstream ss;
		ss << filename << "_" << now << ext;

		return trashDir / ss.str();
	}

	/**
	 * @class	RenameFileCommand
	 * @brief	ファイルのリネーム操作コマンド。Undo時は名前を元に戻します。
	 */
	class RenameFileCommand : public ICommand
	{
	public:
		RenameFileCommand(const std::filesystem::path& oldPath, const std::string& newName)
			: m_OldPath(oldPath)
			, m_NewName(newName)
		{
			m_NewPath = oldPath.parent_path() / newName;
		}

		bool Execute() override
		{
			// 実行: Old -> New
			return EditorFileSystem::RenameFile(m_OldPath, m_NewName);
		}

		void Undo() override
		{
			// 元に戻す: New -> OldName
			EditorFileSystem::RenameFile(m_NewPath, m_OldPath.filename().string());
		}

		const char* GetName() const override { return "Rename Asset"; }

	private:
		std::filesystem::path m_OldPath;
		std::filesystem::path m_NewPath;
		std::string m_NewName;
	};

	/**
	 * @class	MoveFileCommand
	 * @brief	ファイルの移動操作コマンド。Undo時は元の場所に戻します。
	 */
	class MoveFileCommand : public ICommand
	{
	public:
		MoveFileCommand(const std::filesystem::path& source, const std::filesystem::path& destinationDir)
			: m_SourcePath(source)
			, m_DestinationDir(destinationDir)
		{
			m_DestinationPath = destinationDir / source.filename();
		}

		bool Execute() override
		{
			// 実行: Source -> DistDir
			if (m_SourcePath == m_DestinationPath) return false;
			return EditorFileSystem::MoveFile(m_SourcePath, m_DestinationDir);
		}

		void Undo() override
		{
			// 元に戻す: DestPath -> Sourceの親ディレクトリ
			EditorFileSystem::MoveFile(m_DestinationPath, m_SourcePath.parent_path());
		}

		const char* GetName() const override { return "Move Asset"; }

	private:
		std::filesystem::path m_SourcePath;
		std::filesystem::path m_DestinationDir;
		std::filesystem::path m_DestinationPath;	// 移動後のフルパス
	};

	/**
	 * @class	DeleteFileCommand
	 * @brief	ファイルの削除コマンド。
	 */
	class DeleteFileCommand : public ICommand
	{
	public:
		DeleteFileCommand(const std::filesystem::path& path)
			: m_OriginalPath(path)
		{
			m_TrashPath = GetTrashPath(path);
		}

		bool Execute() override
		{
			try
			{
				std::filesystem::rename(m_OriginalPath, m_TrashPath);

				// .metaも削除
				auto srcMeta = m_OriginalPath.string() + ".meta";
				if (std::filesystem::exists(srcMeta))
				{
					std::filesystem::rename(srcMeta, m_TrashPath.string() + ".meta");
				}
				return true;
			}
			catch (...) { return false; }
		}

		void Undo() override
		{
			try
			{
				// 元のディレクトリが消えている場合に備えて再生成
				if (!std::filesystem::exists(m_OriginalPath.parent_path())
				{
					std::filesystem::create_directories(m_OriginalPath.parent_path());
				}

				std::filesystem::rename(m_TrashPath, m_OriginalPath);

				auto trashMeta = m_TrashPath.string() + ".meta";
				if (std::filesystem::exists(trashMeta))
				{
					std::filesystem::rename(trashMeta, m_OriginalPath.string() + ".meta");
				}
			}
			catch (...) {}
		}

		const char* GetName() const override { return "Delete Asset"; }

	private:
		std::filesystem::path m_OriginalPath;
		std::filesystem::path m_TrashPath;
	};

	/**
	 * @class	CreateFileCommand
	 * @brief	ファイルの作成コマンド
	 */
	class CreateFileCommand : public ICommand
	{
	public:
		CreateFileCommand(const std::filesystem::path& path, const std::string& content)
			: m_Path(path)
			, m_Content(content)
		{}

		bool Execute() override
		{
			// 重複チェックなどは呼び出し前で行う前提
			std::ofstream ofs(m_Path);
			if (ofs)
			{
				ofs << m_Content;
				ofs.close();
				return true;
			}
			return false;
		}

		void Undo() override
		{
			// 作成したファイルを削除
			EditorFileSystem::DeleteFile(m_Path);
		}

		const char* GetName() const override { return "Create File"; }

	private:
		std::filesystem::path m_Path;
		std::string m_Content;
	};

	/**
	 * @class	CreateDirectoryCommand
	 * @brief	フォルダの作成コマンド
	 */
	class CreateDirectoryCommand : public ICommand
	{
	public:
		CreateDirectoryCommand(const std::filesystem::path& path)
			: m_Path(path)
		{}

		bool Execute() override
		{
			std::error_code ec;
			return std::filesystem::create_directory(m_Path, ec);
		}

		void Undo() override
		{
			EditorFileSystem::DeleteFile(m_Path);
		}

		const char* GetName() const override { return "Create Folder"; }

	private:
		std::filesystem::path m_Path;
	};
}
