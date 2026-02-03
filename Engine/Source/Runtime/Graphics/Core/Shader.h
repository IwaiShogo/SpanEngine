#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
	enum class ShaderType
	{
		Vertex,
		Pixel
	};

	class Shader
	{
	public:
		// ファイルからシェーダーをロードしてコンパイル
		bool Load(const std::wstring& filename, ShaderType type, const std::string& entryPoint = "Main");

		// GPUに渡すためのバイナリ取得
		ID3DBlob* GetBlob() const { return blob.Get(); }
		D3D12_SHADER_BYTECODE GetBytecode() const { return { blob->GetBufferPointer(), blob->GetBufferSize() }; }

	private:
		ComPtr<ID3DBlob> blob;
	};
}

