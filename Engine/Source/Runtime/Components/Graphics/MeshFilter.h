#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Resources/Mesh.h"

namespace Span
{
	struct MeshFilter
	{
		Mesh* mesh = nullptr;

		MeshFilter() = default;
		MeshFilter(Mesh* m) : mesh(m) {}
	};
}
