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
		std::string target;
		switch (type)
		{
		case ShaderType::Vertex:	target = "vs_5_0"; break;
		case ShaderType::Pixel:		target = "ps_5_0"; break;
		case ShaderType::Compute:	target = "cs_5_0"; break;
		default:					target = "ps_5_0"; break;
		}

		// パス解決
		std::wstring path = L"Shaders/" + filename;
		if (!std::filesystem::exists(path))
		{
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

