/*****************************************************************//**
 * @file	FileDialog.h
 * @brief	OSネイティブのファイル選択ダイアログを表示するユーティリティ。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

#include <optional>

namespace Span
{
	/**
	 * @class	FileDialog
	 * @brief	OSネイティブのファイル選択ダイアログを表示するユーティリティ
	 */
	class FileDialog
	{
	public:
		/**
		 * @brief	ファイルを開くダイアログを表示します。
		 * @param	filter 拡張子フィルター (例: "Scene Files (*.span)\0*.span\0All Files (*.*)\0*.*\0")
		 * @return	選択されたファイルのパス。キャンセル時はstd::nullopt
		 */
		static std::optional<std::string> OpenFile(const char* filter);

		/**
		 * @brief	ファイルを保存するダイアログを表示します。
		 * @param	filter 拡張子
		 * @return	選択されたファイルのパス。キャンセル時はstd::nullopt;
		 */
		static std::optional<std::string> SaveFile(const char* filter);
	};
}
