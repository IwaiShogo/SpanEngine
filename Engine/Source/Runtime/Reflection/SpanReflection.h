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
#include "Runtime/Resource/AssetManager.h"

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

//	🪄 Reflection Macros
// ============================================================

#define SPAN_INSPECTOR_BEGIN(ComponentType) \
	public: \
	using _InspectorSelfType = ComponentType; \
	static const char* _GetInspectorName() { return #ComponentType; } \
	template<typename Visitor> \
	void Reflect(Visitor& visitor) {

#define SPAN_FIELD(Variable, ...) \
	Span::Internal::ProcessField(visitor, #Variable, Variable, __VA_ARGS__);

#define SPAN_INSPECTOR_END() \
	} \
	struct _AutoReg_Inspector { \
		_AutoReg_Inspector() { \
			Span::ComponentRegistry::Register<_InspectorSelfType>( \
				_InspectorSelfType::_GetInspectorName(), \
				/* Draw Func */ \
				[](Span::Entity e, Span::World& w) { \
					if (auto* c = w.GetComponentPtr<_InspectorSelfType>(e)) { \
						bool isRemoved = false; \
						if (Span::ImGuiUI::DrawComponentHeader(_InspectorSelfType::_GetInspectorName(), isRemoved)) { \
							Span::Internal::ImGuiVisitor v; \
							c->Reflect(v); \
							ImGui::TreePop(); \
						} \
						if (isRemoved) w.RemoveComponent<_InspectorSelfType>(e); \
					} \
				}, \
				/* Add Func */ \
				[](Span::Entity e, Span::World& w) { \
					if (!w.HasComponent<_InspectorSelfType>(e)) w.AddComponent<_InspectorSelfType>(e); \
				}, \
				/* Has Func */ \
				[](Span::Entity e, Span::World& w) { \
					return w.HasComponent<_InspectorSelfType>(e); \
				}, \
				/* Serialize Func */ \
				[](Span::Entity e, Span::World& w, nlohmann::ordered_json& j) { \
					if (auto* c = w.GetComponentPtr<_InspectorSelfType>(e)) { \
						Span::Internal::JsonSerializeVisitor v(j); \
						c->Reflect(v); \
					} \
				}, \
				/* Deserialize Func */ \
				[](Span::Entity e, Span::World& w, const nlohmann::ordered_json& j) { \
					if (auto* c = w.GetComponentPtr<_InspectorSelfType>(e)) { \
						Span::Internal::JsonDeserializeVisitor v(j); \
						c->Reflect(v); \
					} \
				} \
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
	// 前方宣言
	template <typename T>
	void DrawField(const char* label, T& value, const std::vector<Attribute>& attrs);

	template <typename Visitor, typename T, typename... Attrs>
	void ProcessField(Visitor& visitor, const char* name, T& value, Attrs&&... attrs)
	{
		std::vector<Span::Attribute> attrList = { std::forward<Attrs>(attrs)... };
		visitor.Visit(name, value, attrList);
	}

	// 🐾 Visitor Classes (UI描画とJSON処理の自動分岐)
	// ============================================================

	// 1. UI描画用ビジター
	struct ImGuiVisitor
	{
		template<typename T>
		void Visit(const char* name, T& value, const std::vector<Attribute>& attrs)
		{
			Span::Internal::DrawField(name, value, attrs);
		}
	};

	// 2. JSON保存用ビジター
	struct JsonSerializeVisitor
	{
		nlohmann::ordered_json& j;
		JsonSerializeVisitor(nlohmann::ordered_json& jsonRef) : j(jsonRef) {}

		template<typename T>
		void Visit(const char* name, T& value, const std::vector<Attribute>& /* attrs */)
		{
			if constexpr (std::is_enum_v<T>) {
				j[name] = static_cast<int>(value);
			}
			else if constexpr (Span::is_fixed_string_v<T>) {
				j[name] = value.Data;
			}
			else if constexpr (std::is_same_v<T, Vector3>) {
				j[name] = { value.x, value.y, value.z };
			}
			else if constexpr (std::is_same_v<T, Quaternion>) {
				j[name] = { value.x, value.y, value.z, value.w };
			}
			else if constexpr (std::is_same_v<T, Span::Material*>) {
				j[name] = value ? value->Handle : 0;
			}
			else if constexpr (std::is_pointer_v<T>) {
				// アセットポインタ等は現状スキップ
			}
			else if constexpr (std::is_same_v<T, Span::Entity>) {
				j[name] = value.ToUInt64();
			}
			else if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
				j[name] = value;
			}
			else {

			}
		}
	};

	// 3. JSON読み込み用ビジター
	struct JsonDeserializeVisitor
	{
		const nlohmann::ordered_json& j;
		JsonDeserializeVisitor(const nlohmann::ordered_json& jsonRef) : j(jsonRef) {}

		template<typename T>
		void Visit(const char* name, T& value, const std::vector<Attribute>& /* attrs */)
		{
			// 変数名がJSON内に存在するかチェック
			if (!j.contains(name)) return;

			if constexpr (std::is_enum_v<T>) {
				value = static_cast<T>(j[name].get<int>());
			}
			else if constexpr (Span::is_fixed_string_v<T>) {
				std::string str = j[name].get<std::string>();
				strncpy_s(value.Data, str.c_str(), value.Capacity());
			}
			else if constexpr (std::is_same_v<T, Vector3>) {
				const auto& arr = j[name];
				if (arr.is_array() && arr.size() >= 3) {
					value.x = arr[0]; value.y = arr[1]; value.z = arr[2];
				}
			}
			else if constexpr (std::is_same_v<T, Quaternion>) {
				const auto& arr = j[name];
				if (arr.is_array() && arr.size() >= 4) {
					value.x = arr[0]; value.y = arr[1]; value.z = arr[2]; value.w = arr[3];
				}
			}
			else if constexpr (std::is_same_v<T, Span::Material*>) {
				uint64_t guid = j[name].get<uint64_t>();
				if (guid != 0)
				{
					value = Span::AssetManager::Get().GetMaterial(guid).get();
				}
				else
				{
					value = nullptr;
				}
			}
			else if constexpr (std::is_pointer_v<T>) {
				// スキップ
			}
			else if constexpr (std::is_same_v<T, Span::Entity>) {
				value = Span::Entity::Null;
			}
			else if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
				value = j[name].get<T>();
			}
			else {

			}
		}
	};

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
		// 1. 基本型
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
		// 2. 文字列
		else if constexpr (Span::is_fixed_string_v<T>) {
			ImGui::InputText(label, value.Data, value.Capacity());
		}
		else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_all_extents_t<T>, char>) {
			ImGui::InputText(label, value, sizeof(T));
		}
		// 3. Asset Pointers (Drag & Drop Slots)
		else if constexpr (std::is_same_v<T, Span::Texture*>) {
			ImGuiUI::DrawTextureSlot(label, value);
		}
		else if constexpr (std::is_same_v<T, Span::Mesh*>) {
			ImGuiUI::DrawMeshSlot(label, value);
		}
		else if constexpr (std::is_same_v<T, Span::Material*>) {
			ImGuiUI::DrawMaterialSlot(label, value);
		}
		// 4. Quaternion (Euler変換)
		else if constexpr (std::is_same_v<T, Quaternion>) {
			Vector3 euler = value.ToEuler();
			Vector3 deg = { ToDegrees(euler.x), ToDegrees(euler.y), ToDegrees(euler.z) };

			// 回転は -180~180 に正規化して表示
			if (deg.x > 180.0f) deg.x -= 360.0f; if (deg.x < -180.f) deg.x += 360.f;
			if (deg.y > 180.f) deg.y -= 360.f; if (deg.y < -180.f) deg.y += 360.f;
			if (deg.z > 180.f) deg.z -= 360.f; if (deg.z < -180.f) deg.z += 360.f;

			if (ImGuiUI::DrawVec3Control(label, deg)) {
				value = Quaternion::FromEuler(ToRadians(deg.x), ToRadians(deg.y), ToRadians(deg.z));
			}
		}
		// 5. Enum (Magic Enum)
		else if constexpr (std::is_enum_v<T>) {
			// 現在のEnum名の取得
			std::string_view currentName = magic_enum::enum_name(value);
			if (currentName.empty()) currentName = "Unknown";

			if (ImGui::BeginCombo(label, currentName.data()))
			{
				// Enumの全要素を列挙
				constexpr auto values = magic_enum::enum_values<T>();
				for (const auto& v : values)
				{
					bool isSelected = (value == v);
					std::string_view name = magic_enum::enum_name(v);

					if (ImGui::Selectable(name.data(), isSelected))
					{
						value = v;
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		// 6. 未対応
		else {
			ImGui::TextDisabled("%s (Not Supported)", label);
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
