/*****************************************************************//**
 * @file	FixedString.h
 * @brief	固定長文字列ラッパー (POD)
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include <string_view>

namespace Span
{
	/**
	 * @struct	FixedString
	 * @brief	📦 固定長バッファを持つ文字列クラス。
	 *
	 * @details
	 * - ヒープ割り当てを行わないため、ECSのコンポーネントとして安全に使用できます。
	 * - std::string と互換性のあるインターフェースを提供します。
	 * @tparam	N 最大文字数 (ヌル終端を含むバッファサイズ)
	 */
	template<size_t N>
	struct FixedString
	{
		char Data[N];

		// --- Constructors ---
		FixedString()
		{
			Data[0] = '\0';
		}

		FixedString(const char* str)
		{
			Set(str);
		}

		FixedString(const std::string& str)
		{
			Set(str.c_str());
		}

		// --- Assignment ---
		FixedString& operator=(const char* str)
		{
			Set(str);
			return *this;
		}

		FixedString& operator=(const std::string& str)
		{
			Set(str.c_str());
			return *this;
		}

		// --- Accessors ---
		const char* c_str() const { return Data; }
		char* data() { return Data; }

		bool Empty() const { return Data[0] == '\0'; }
		static constexpr size_t Capacity() { return N; }

		// --- Utilities ---
		void Set(const char* str)
		{
			if (str)
			{
				strncpy_s(Data, N, str, _TRUNCATE);
			}
			else
			{
				Data[0] = '\0';
			}
		}

		// --- Operators ---
		operator std::string() const { return std::string(Data); }

		bool operator==(const FixedString& other) const { return strcmp(Data, other.Data) == 0; }
		bool operator==(const char* other) const { return strcmp(Data, other) == 0; }
		bool operator==(const std::string& other) const { return other == Data; }

		bool operator!=(const FixedString& other) const { return !(*this == other); }
	};

	// よく使うサイズのエイリアス定義
	using String32 = FixedString<32>;
	using String64 = FixedString<64>;
	using String256 = FixedString<256>;
}
