#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Resources/Material.h"

namespace Span
{
	struct MeshRenderer
	{
		Material* material = nullptr;
		bool castShadows = true;
		bool receiveShadows = true;

		MeshRenderer() = default;
		MeshRenderer(Material* m) : material(m) {}
	};
}

