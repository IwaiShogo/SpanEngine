/*****************************************************************//**
 * @file	AssetSerializer.h
 * @brief	.metaファイルの読み書きを担当。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

#include "AssetMetadata.h"

namespace Span
{
	/**
	 * @class	AssetSerializer
	 * @brief	.metaファイルの生成、読み込み、保存を担当する静的クラス。
	 */
	class AssetSerializer
	{
	public:
		/**
		 * @brief	指定したアセットのメタデータを取得します。
		 * .metaファイルが存在しない場合は、新規にGUIDを生成して作成します。
		 * 
		 * @param	assetPath アセットファイルのパス
		 * @return	読み込まれた (または生成された) メタデータ
		 */
		static AssetMetadata LoadOrCreateMetadata(const std::filesystem::path& assetPath);

		/**
		 * @brief	メタデータをファイルに保存する。
		 * 
		 * @param	assetPath 元のアセットのパス (例: "A.png" -> "A.png.meta" に保存)
		 * @param	metadata 保存するメタデータ
		 */
		static void SaveMetadata(const std::filesystem::path& assetPath, const AssetMetadata& metadata);

	private:
		/**
		 * @brief	拡張子から汗ッとタイプを推論します。
		 */
		static AssetType DeduceTypeFromExtension(const std::filesystem::path& path);
	};
}
