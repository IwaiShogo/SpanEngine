/*****************************************************************//**
 * @file	ICommand.h
 * @brief	Undo/Redoシステムの為のコマンドインターフェース定義。
 * 
 * @details
 * 全ての編集操作は子のインターフェースを実装したクラスとして定義し,
 * CommandHistoryを通じて実行することでUndo/Redoが可能になります。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include <string>

namespace Span
{
	/**
	 * @class	ICommand
	 * @brief	元に戻す(Undo) / やり直し(Redo) をサポートするための操作基底クラス。
	 */
	class ICommand
	{
	public:
		virtual ~ICommand() = default;

		/**
		 * @brief	コマンドを実行します。
		 * @return	成功したら true
		 */
		virtual bool Execute() = 0;

		/**
		 * @brief	コマンドの実行を取り消し、元の状態に戻します。
		 */
		virtual void Undo() = 0;

		/**
		 * @brief	UI表示用のコマンド名を取得します (例: "Move File", "Rename Asset")。
		 */
		virtual const char* GetName() const = 0;
	};
}
