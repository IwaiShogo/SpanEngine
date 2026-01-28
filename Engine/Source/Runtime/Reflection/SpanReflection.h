#pragma once
#include "SpanAttributes.h"
#include "Editor/ImGui/ImGuiUI.h"
#include <vector>
#include <string>
#include <type_traits>
#include <cfloat>

#include "ComponentRegistry.h" 

// -------------------------------------------------------------------------
//	SPAN_INSPECTOR_BEGIN / END
// -------------------------------------------------------------------------

#define SPAN_INSPECTOR_BEGIN(ComponentType) \
	public: \
	using _InspectorSelfType = ComponentType; \
	static const char* _GetInspectorName() { return #ComponentType; } \
	void OnGui() { \
		if (Span::ImGuiUI::DrawComponentHeader(#ComponentType)) {

#define SPAN_INSPECTOR_END() \
			ImGui::TreePop(); \
		} \
	} \
	struct _AutoReg_Inspector { \
		_AutoReg_Inspector() { \
			Span::ComponentRegistry::Register<_InspectorSelfType>( \
				_InspectorSelfType::_GetInspectorName(), \
				[](_InspectorSelfType& t){ t.OnGui(); } \
			); \
		} \
	}; \
	inline static _AutoReg_Inspector _autoreg_inspector;

// -------------------------------------------------------------------------
//	SPAN_FIELD
// -------------------------------------------------------------------------

#define SPAN_FIELD(Variable, ...) \
	{ \
		using namespace Span; \
		std::vector<Attribute> attrs = { __VA_ARGS__ }; \
		Span::Internal::DrawField(#Variable, Variable, attrs); \
	}

namespace Span::Internal
{
	// îƒópï`âÊä÷êî
	template<typename T>
	void DrawField(const char* label, T& value, const std::vector<Attribute>& attrs)
	{
		// ëÆê´âêÕ
		std::string tooltip;
		float minV = -FLT_MAX, maxV = FLT_MAX;
		bool hasRange = false;
		bool hasMin = false; // Åöí«â¡
		bool isReadOnly = false;

		for (const auto& a : attrs) {
			if (a.Type == AttributeType::HideInInspector) return;
			if (a.Type == AttributeType::Header) { ImGui::Spacing(); ImGui::SeparatorText(a.StringValue.c_str()); }
			if (a.Type == AttributeType::Space) ImGui::Spacing();
			if (a.Type == AttributeType::Tooltip) tooltip = a.StringValue;

			if (a.Type == AttributeType::Range) {
				hasRange = true; minV = a.FloatValue1; maxV = a.FloatValue2;
			}
			if (a.Type == AttributeType::ReadOnly) isReadOnly = true;

			if (a.Type == AttributeType::Min) {
				hasMin = true; // Åöí«â¡
				minV = a.FloatValue1;
				// ï`âÊëOÇÃéñëOÉ`ÉFÉbÉN
				if constexpr (std::is_arithmetic_v<T>) {
					if (value < static_cast<T>(a.FloatValue1)) value = static_cast<T>(a.FloatValue1);
				}
			}
		}

		if (isReadOnly) ImGui::BeginDisabled();

		// --- å^ï ï`âÊ ---
		if constexpr (std::is_same_v<T, float>) {
			if (hasRange) ImGui::SliderFloat(label, &value, minV, maxV);
			else ImGui::DragFloat(label, &value, 0.1f, minV, maxV);
		}
		else if constexpr (std::is_same_v<T, int>) {
			ImGui::DragInt(label, &value, 1.0f, (int)minV, (int)maxV);
		}
		else if constexpr (std::is_same_v<T, bool>) {
			ImGui::Checkbox(label, &value);
		}
		else if constexpr (std::is_same_v<T, Vector3>) {
			ImGuiUI::DrawVec3Control(label, value);
		}

		if (isReadOnly) ImGui::EndDisabled();

		if constexpr (std::is_arithmetic_v<T>)
		{
			if (hasMin && value < static_cast<T>(minV)) value = static_cast<T>(minV);
			if (hasRange)
			{
				if (value < static_cast<T>(minV)) value = static_cast<T>(minV);
				if (value > static_cast<T>(maxV)) value = static_cast<T>(maxV);
			}
		}

		if (!tooltip.empty() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip.c_str());
	}
}