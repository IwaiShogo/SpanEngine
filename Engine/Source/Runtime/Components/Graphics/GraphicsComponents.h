#pragma once
#include "Graphics/Resources/Mesh.h"
#include "Graphics/Resources/Material.h"

namespace Span
{
	struct MeshRenderer
	{
		Mesh* MeshAsset = nullptr;
		Material* MaterialAsset = nullptr;
	};
}
