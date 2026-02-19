/*****************************************************************//**
 * @file	DirectoryWatcher.h
 * @brief	ディレクトリの変更を監視するクラス
 * 
 * @details	
 * std::filesystem を使用して、指定されたディレクトリ内の
 * ファイルの追加、削除、変更、リネームをバックグラウンドで監視します。
 * 変更が検知された場合、登録されたコールバック関数を呼び出します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

namespace Span
{
	/**
	 * @class	DirectoryWatcher
	 * @brief	特定のディレクトリを監視し、変更を通知するクラス。
	 * 
	 * @details
	 * エディタのホットリロード機能 (Project Browserの自動更新など) に利用されます。
	 * バックグラウンドスレッドで定期的にディレクトリをスキャンし、差分を検知します。
	 */
	class DirectoryWatcher
	{
	public:
		/**
		 * @brief	変更検知時に発火するコールバックの型
		 */
		using ActionCallback = std::function<void()>;

		/**
		 * @brief	コンストラクタ
		 * @param	directoryToWatch 監視対象のディレクトリパス
		 * @param	onAction 変更検知時に呼ばれるコールバック
		 */
		DirectoryWatcher(const std::filesystem::path& directoryToWatch, ActionCallback onAction);

		/**
		 * @brief	デストラクタ。監視スレッドを安全に停止します。
		 */
		~DirectoryWatcher();

		/**
		 * @brief	監視対象のディレクトリを変更します。
		 * @param	newDirectory 新しい監視対象のディレクトリパス
		 */
		void SetDirectory(const std::filesystem::path& newDirectory);

	private:
		/**
		 * @brief	バックグラウンドで実行される監視ループ
		 */
		void WatchLoop();

	private:
		std::filesystem::path m_CurrentDirectory;
		ActionCallback m_OnAction;

		std::thread m_WatchThread;
		std::atomic<bool> m_IsRunning;
		std::mutex m_Mutex;

		// ファイルパスと最終更新時間のキャッシュ
		std::unordered_map<std::string, std::filesystem::file_time_type> m_FileCache;
	};
}
