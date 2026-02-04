/*****************************************************************//**
 * @file	Shader.h
 * @brief	HLSLシェーダーのロードとコンパイル管理。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	/// @brief	シェーダーの種類
	enum class ShaderType
	{
		Vertex,	///< 頂点シェーダー (vs_5_0)
		Pixel	///< ピクセルシェーダー (vs_5_0)
	};

	/**
	 * @class	Shader
	 * @brief	📜 HLSLシェーダーファイルのコンパイルとバイナリ保持。
	 * 
	 * @details
	 * 実行時コンパイル (D3DCompileFromFile) を行います。
	 * コンパイルエラーが発生した場合は、Visual Studioの出力ウィンドウとログに詳細を表示します。
	 * 
	 * ### 📁 パス解決
	 * 実行ファイルのカレントディレクトリを基準に `Shaders/`フォルダを探します。
	 * 見つからない場合は親ディレクトリへ遡って探索するため、エディタ実行時でも安心です。
	 */
	class Shader
	{
	public:
		/**
		 * @brief	ファイルからシェーダーをロードしてコンパイルします。
		 * @param	filename ファイル名 (例: "Basic.hlsl")
		 * @param	type シェーダータイプ (Vertex or Pixel)
		 * @param	entryPoint エントリーポイント関数名 (Default: "Main")
		 * @return	成功なら true
		 */
		bool Load(const std::wstring& filename, ShaderType type, const std::string& entryPoint = "Main");

		/// @brief	コンパイル済みバイナリ (Blob) を取得
		ID3DBlob* GetBlob() const { return blob.Get(); }

		/// @brief	パイプラインステート生成用のバイトコード構造体を取得
		D3D12_SHADER_BYTECODE GetBytecode() const { return { blob->GetBufferPointer(), blob->GetBufferSize() }; }

	private:
		ComPtr<ID3DBlob> blob;
	};
}

