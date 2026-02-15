/*****************************************************************//**
 * @file	SpanReflection.h
 * @brief	コンポーネント定義用のリフレクションマクロ。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "SpanAttributes.h"
#include "ComponentRegistry.h"
#include "Editor/ImGui/ImGuiUI.h"
#include "Core/Containers/FixedString.h"
#include <vector>
#include <string>
#include <type_traits>
#include <cfloat>


//	🪄 Reflection Macros
// ============================================================

/**
 * @def		SPAN_INSPECTOR_BEGIN(ComponentType)
 * @brief	インスペクターUI定義の開始マクロ
 *
 * @details
 * 構造体の中に `OnGui` 関数と、自動登録用の静的構造体 `_AutoReg_Inspector` を生成します。
 */
#define SPAN_INSPECTOR_BEGIN(ComponentType) \
	public: \
	using _InspectorSelfType = ComponentType; \
	static const char* _GetInspectorName() { return #ComponentType; } \
	/* UI描画関数 (InspectorPanelから呼ばれる) */ \
	void OnGui(Span::Entity entity, Span::World& world) { \
		bool isRemoved = false; \
		/* ヘッダー描画 (折り畳み & 右クリックメニュー) */ \
		if (Span::ImGuiUI::DrawComponentHeader(#ComponentType, isRemoved)) {

/**
 * @def		SPAN_FIELD(Variable, Label, ...)
 * @brief	変数をインスペクターに表示します。
 * @param	Variable メンバ変数名
 * @param	Label UI上のラベル名
 * @param	... 属性リスト (Range(0, 1), Tooltip("Help") 等)
 */
#define SPAN_FIELD(Variable, ...) \
	{ \
		using namespace Span; \
		std::vector<Attribute> attrs = { __VA_ARGS__ }; \
		Span::Internal::DrawField(#Variable, Variable, attrs); \
	}

/**
 * @def		SPAN_INSPECTOR_END()
 * @brief	インスペクターUI定義の終了マクロ。
 */
#define SPAN_INSPECTOR_END() \
			ImGui::TreePop(); \
		} \
		if (isRemoved) { \
			world.RemoveComponent<_InspectorSelfType>(entity); \
		} \
	} \
	/* 自動登録用構造体 (main前にコンストラクタが走る) */ \
	struct _AutoReg_Inspector { \
		_AutoReg_Inspector() { \
			Span::ComponentRegistry::Register<_InspectorSelfType>( \
				_InspectorSelfType::_GetInspectorName(), \
				[](_InspectorSelfType& t, Span::Entity e, Span::World& w){ t.OnGui(e, w); } \
			); \
		} \
	}; \
	inline static _AutoReg_Inspector _autoreg_inspector;

namespace Span
{
	// テンプレートメタプログラミング用: TがFixedString<N>かどうか判定する。
	template<typename T> struct is_fixed_string : std::false_type {};

	template<size_t N>
	struct is_fixed_string<FixedString<N>> : std::true_type {};

	template<typename T>
	inline constexpr bool is_fixed_string_v = is_fixed_string<T>::value;
}

namespace Span::Internal
{
	/**
	 * @brief	型に応じた描画ヘルパー関数
	 */
	template<typename T>
	void DrawField(const char* label, T& value, const std::vector<Attribute>& attrs)
	{
		// 属性解析
		std::string tooltip;
		float minV = -FLT_MAX, maxV = FLT_MAX;
		bool hasRange = false;
		bool hasMin = false;
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
				hasMin = true;
				minV = a.FloatValue1;
				// 描画前の事前チェック
				if constexpr (std::is_arithmetic_v<T>) {
					if (value < static_cast<T>(a.FloatValue1)) value = static_cast<T>(a.FloatValue1);
				}
			}
		}

		if (isReadOnly) ImGui::BeginDisabled();

		// --- 型別描画 ---
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
		else if constexpr (Span::is_fixed_string_v<T>) {
			ImGui::InputText(label, value.Data, value.Capacity());
		}
		else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_all_extents_t<T>, char>) {
			ImGui::InputText(label, value, sizeof(T));
		}
		else if constexpr (std::is_same_v<T, Span::Texture*>) {
			Span::ImGuiUI::DrawTextureSlot(label, value);
		}
		else if constexpr (std::is_same_v<T, Span::Mesh*>) {
			Span::ImGuiUI::DrawMeshSlot(label, value);
		}
		else if constexpr (std::is_same_v<T, Span::Material*>) {
			Span::ImGuiUI::DrawMaterialSlot(label, value);
		}
		else if constexpr (std::is_enum_v<T>) {
			int val = static_cast<int>(value);
			if (ImGui::DragInt(label, &val)) value = static_cast<T>(val);
		}
		else {
			ImGui::TextDisabled("%s (Unknown)", label);
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
