/*****************************************************************//**
 * @file	MeshFilter.h
 * @brief	描画する形状 (メッシュ) への参照。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Graphics/Resources/Mesh.h"
#include "Runtime/Reflection/SpanReflection.h"

namespace Span
{
	/**
	 * @struct	MeshFilter
	 * @brief	📦 描画に使用する3Dモデルデータ (頂点バッファ) を保持するコンポーネント。
	 *
	 * @details
	 * `MeshRenderer` とセットで使用されます。
	 * 実際のメッシュデータはリソースとして共有され、ここではポインタのみを保持します。
	 */
	struct MeshFilter
	{
		Mesh* mesh = nullptr;

		MeshFilter() = default;
		MeshFilter(Mesh* m) : mesh(m) {}

		SPAN_INSPECTOR_BEGIN(MeshFilter)
			if (mesh) {
				ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Mesh Loaded (0x%p)", mesh);
			}
			else {
				ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Empty (Null)");
			}
		SPAN_INSPECTOR_END()
	};
}

