/*****************************************************************//**
 * @file	SpanAttributes.h
 * @brief	インスペクタ表示用の属性 (Attribute) 定義。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

namespace Span
{
	/// @brief	属性の種類
	enum class AttributeType
	{
		None,
		Range,				///< スライダー表示 (min ~ max)
		Min,				///< 最小値制限
		Tooltip,			///< マウスホバー時のヒント
		Header,				///< 太字見出し
		Space,				///< 空白行
		HideInInspector,	///< 表示しない
		ReadOnly,			///< 編集不可
		MultiLine			///< 複数行テキスト
	};

	/**
	 * @struct	Attribute
	 * @brief	🏷️ 変数に付与するメタデータ。
	 * 
	 * @details
	 * `SpanReflection` マクロで使用され、UIの挙動を制御します。
	 */
	struct Attribute
	{
		AttributeType Type = AttributeType::None;

		// 汎用データ
		float FloatValue1 = 0.0f;
		float FloatValue2 = 0.0f;
		std::string StringValue;

		Attribute() = default;
	};

	// 🏗️ Attribute Constructors (Helper Functions)
	// ============================================================

	/// @brief	変数をスライダーで編集できるようにします。
	inline Attribute Range(float min, float max)
	{
		Attribute attr; attr.Type = AttributeType::Range; attr.FloatValue1 = min; attr.FloatValue2 = max; return attr;
	}

	/// @brief	変数の最小値を制限します。
	inline Attribute Min(float min)
	{
		Attribute attr; attr.Type = AttributeType::Min; attr.FloatValue1 = min; return attr;
	}

	/// @brief	変数にツールチップ (説明文) を追加します。
	inline Attribute Tooltip(const char* text)
	{
		Attribute attr; attr.Type = AttributeType::Tooltip; attr.StringValue = text; return attr;
	}

	/// @brief	変数の上にヘッダー文字を表示します。
	inline Attribute Header(const char* header)
	{
		Attribute attr; attr.Type = AttributeType::Header; attr.StringValue = header; return attr;
	}

	/// @brief	変数の上にスペース (余白) を追加します。
	inline Attribute Space()
	{
		Attribute attr; attr.Type = AttributeType::Space; return attr;
	}

	/// @brief	インスペクターに表示しません (シリアライズのみしたい場合など)。
	inline Attribute HideInInspector()
	{
		Attribute attr; attr.Type = AttributeType::HideInInspector; return attr;
	}

	/// @brief	値を表示しますが、編集は出来ないようにします。
	inline Attribute ReadOnly()
	{
		Attribute attr; attr.Type = AttributeType::ReadOnly; return attr;
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

