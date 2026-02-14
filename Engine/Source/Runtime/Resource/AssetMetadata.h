/*****************************************************************//**
 * @file	AssetMetadata.h
 * @brief	アセットのメタデータを定義する構造体。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

namespace Span
{
	// アセットを一意に識別するID (64bit整数)
	using AssetHandle = uint64_t;

	/**
	 * @enum	AssetType
	 * @brief	アセットの種類を識別するための列挙型。
	 */
	enum class AssetType
	{
		None = 0,
		Texture,	///< 画像 (.pnt, .jpg)
		Mesh,		///< モデル (.fbx, .obj)
		Material,	///< マテリアル (.mat)
		Scene,		///< シーン (.span)
		Script,		///< コード (.h, .cpp)
		Audio		///< 音声 (.wav, .mp3)
	};

	/**
	 * @struct	AssetMetadata
	 * @brief	.metaファイルに保存されるデータ構造。
	 */
	struct AssetMetadata
	{
		AssetHandle Handle = 0;				///< アセット固有のID (GUID)
		AssetType Type = AssetType::None;	///< アセットの種類

		// 将来的にImportSettingsを追加
		// bool sRGB = true;
		// int compression = 0;

		bool IsValid() const { return Handle != 0; }
	};
}
