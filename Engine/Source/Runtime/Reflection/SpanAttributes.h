#pragma once

namespace Span
{
	enum class AttributeType
	{
		None,
		Range,
		Min,
		Tooltip,
		Header,
		Space,
		HideInInspector,
		ReadOnly,
		MultiLine
	};

	struct Attribute
	{
		AttributeType Type = AttributeType::None;

		// 汎用データ
		float FloatValue1 = 0.0f;
		float FloatValue2 = 0.0f;
		std::string StringValue;

		Attribute() = default;
	};

	// ---ユーザーが使う属性定義 ---

	inline Attribute Range(float min, float max)
	{
		Attribute attr; attr.Type = AttributeType::Range; attr.FloatValue1 = min; attr.FloatValue2 = max; return attr;
	}

	inline Attribute Min(float min)
	{
		Attribute attr; attr.Type = AttributeType::Min; attr.FloatValue1 = min; return attr;
	}

	inline Attribute Tooltip(const char* text)
	{
		Attribute attr; attr.Type = AttributeType::Tooltip; attr.StringValue = text; return attr;
	}

	inline Attribute Header(const char* header)
	{
		Attribute attr; attr.Type = AttributeType::Header; attr.StringValue = header; return attr;
	}

	inline Attribute Space()
	{
		Attribute attr; attr.Type = AttributeType::Space; return attr;
	}

	inline Attribute HideInInspector()
	{
		Attribute attr; attr.Type = AttributeType::HideInInspector; return attr;
	}

	// Unityの SerializeField = 「privateでも表示する」だが、C++はpublic前提なので「読み取り専用」として扱うか、単なるマーカーにする
	inline Attribute SerializeField()
	{
		Attribute attr; attr.Type = AttributeType::None; return attr;
	}

	inline Attribute TextArea()
	{
		Attribute attr; attr.Type = AttributeType::MultiLine; return attr;
	}
}
