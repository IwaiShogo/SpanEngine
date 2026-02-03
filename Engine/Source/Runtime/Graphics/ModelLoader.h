#pragma once
#include "Resources/Mesh.h"

namespace Span
{
	class ModelLoader
	{
	public:
		static std::vector<Mesh*> Load(ID3D12Device* device, const std::string& filepath);

	private:
		static Mesh* ProcessMesh(ID3D12Device* device, aiMesh* mesh, const aiScene* scene);
	};
}

