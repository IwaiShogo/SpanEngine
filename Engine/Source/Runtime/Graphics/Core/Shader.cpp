#include "Shader.h"

namespace Span
{
	bool Shader::Load(const std::wstring& filename, ShaderType type, const std::string& entryPoint)
	{
		// 開発中はデバッグ情報を付与
		UINT compileFlags = 0;
#if defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		// シェーダーモデルの指定 (vs_5_0, ps_5_0)
		std::string target = (type == ShaderType::Vertex) ? "vs_5_0" : "ps_5_0";

		// パス解決 (実行ファイルからの相対パス)
		// ビルド後の構成に合わせて調整が必要だが、今回はシンプルに "Shaders/" を探す
		std::wstring path = L"Shaders/" + filename;
		if (!std::filesystem::exists(path))
		{
			// デバッガから実行した場合、カレントディレクトリがズレることがあるため親も探す
			path = L"../../Engine/Shaders/" + filename;
		}

		ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3DCompileFromFile(
			path.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entryPoint.c_str(),
			target.c_str(),
			compileFlags,
			0,
			&blob,
			&errorBlob
		);

		if (FAILED(hr))
		{
			if (errorBlob)
			{
				SPAN_ERROR("Shader Compile Error: %s", (char*)errorBlob->GetBufferPointer());
			}
			else
			{
				SPAN_ERROR("Shader File Not Found: %ls", filename.c_str());
			}
			return false;
		}

		return true;
	}
}

