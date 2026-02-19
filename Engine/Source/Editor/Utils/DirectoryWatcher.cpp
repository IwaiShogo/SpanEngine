/*****************************************************************//**
 * @file	DirectoryWatcher.cpp
 * @brief	DirectoryWatcherの実装
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "DirectoryWatcher.h"

namespace Span
{
	DirectoryWatcher::DirectoryWatcher(const std::filesystem::path& directoryToWatch, ActionCallback onAction)
		: m_CurrentDirectory(directoryToWatch)
	{
		SetDirectory(directoryToWatch);
	}

	DirectoryWatcher::~DirectoryWatcher()
	{
		m_IsRunning = false;
		if (m_WatchThread.joinable())
		{
			m_WatchThread.join();
		}
	}

	void DirectoryWatcher::SetDirectory(const std::filesystem::path& newDirectory)
	{
		// 1. スレッドの停止要求を出す
		m_IsRunning = false;

		// 2. 監視スレッドの終了を待つ
		if (m_WatchThread.joinable())
		{
			m_WatchThread.join();
		}

		// 3. スレッドが完全に停止した状態で、安全にデータを更新する
		{
			std::lock_guard<std::mutex> lock(m_Mutex);

			m_CurrentDirectory = newDirectory;
			m_FileCache.clear();

			// 初回状態をキャッシュ
			if (std::filesystem::exists(m_CurrentDirectory))
			{
				for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
				{
					m_FileCache[entry.path().string()] = std::filesystem::last_write_time(entry);
				}
			}
		}

		// スレッド再開
		m_IsRunning = true;
		m_WatchThread = std::thread(&DirectoryWatcher::WatchLoop, this);
	}

	void DirectoryWatcher::WatchLoop()
	{
		while (m_IsRunning)
		{
			// 500ミリ秒毎にポーリング
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			std::lock_guard<std::mutex> lock(m_Mutex);
			if (!std::filesystem::exists(m_CurrentDirectory)) continue;

			bool changed = false;
			std::unordered_map<std::string, std::filesystem::file_time_type> currentFiles;

			// 現在のファイル状態を取得
			for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				currentFiles[entry.path().string()] = std::filesystem::last_write_time(entry);
			}

			// 1. 追加・変更の検知
			for (const auto& [path, time] : currentFiles)
			{
				auto it = m_FileCache.find(path);
				if (it == m_FileCache.end() || it->second != time)
				{
					changed = true;
					break;
				}
			}

			// 2. 削除の検知
			if (!changed)
			{
				for (const auto& [path, time] : m_FileCache)
				{
					if (currentFiles.find(path) == currentFiles.end())
					{
						changed = true;
						break;
					}
				}
			}

			// 変更があればコールバックを発火してキャッシュ更新
			if (changed)
			{
				m_FileCache = currentFiles;
				if (m_OnAction && m_IsRunning)
				{
					m_OnAction();
				}
			}
		}
	}
}
