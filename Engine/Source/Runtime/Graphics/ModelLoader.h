/*****************************************************************//**
 * @file	ModelLoader.h
 * @brief	3Dモデルファイル(.fbx, .obj)のインポート処理。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Resources/Mesh.h"

namespace Span
{
	/**
	 * @class	ModelLoader
	 * @brief	🏗️ Assimpライブラリを使用したモデルローダー。
	 * 
	 * @details
	 * 外部形式のファイル読み込み、エンジンの `Mesh` 形式に変換します。
	 * 将来的には、読み込み時間を短縮するために独自バイナリ形式(.spanmesh)へのキャッシュ機能を実装予定。
	 */
	class ModelLoader
	{
	public:
		/**
		 * @brief	モデルファイルをロードし、メッシュリストを返します
		 * @param	device メッシュ生成用のデバイス
		 * @param	filepath ファイルパス
		 * @return	生成されたメッシュのポインタ配列
		 */
		static std::vector<Mesh*> Load(ID3D12Device* device, const std::string& filepath);

	private:
		static Mesh* ProcessMesh(ID3D12Device* device, aiMesh* mesh, const aiScene* scene);
	};
}

