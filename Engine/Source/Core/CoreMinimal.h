/*****************************************************************//**
 * @file	CoreMinimal.h
 * @brief	エンジンの標準インクルードファイル。
 * 
 * @details
 * ほぼ全てのソースファイルでインクルードされる前提の軽量ヘッダーです。
 * 基本的な型定義、STL、Windows APIの設定、DirectXの参照を含みます。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

// 1. Platform Settings
// ============================================================

// マクロの衝突を防ぐための定義
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Windows API
#include <Windows.h>
#include <wrl.h>
#include <wrl/client.h>

// 2. Standard Library (STL)
// ============================================================

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>
#include <chrono>
#include <cstdio>
#include <cassert>
#include <typeindex>
#include <filesystem>
#include <set>

// 3. DirectX 12 & Math
// ============================================================

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// リンクライブラリ
#pragma (lib, "d3d12.lib")
#pragma (lib, "dxgi.lib")
#pragma (lib, "d3dcompiler.lib")

// 4. Assimp
// ============================================================

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// 5. Common Aliases & Macros
// ============================================================

using namespace Microsoft::WRL; // ComPtr用
using namespace DirectX;		// XMMATRIX, XMFLOAT3等用

namespace Span
{
	// Rust/C#風の型エイリアス
	using int8	 = int8_t;
	using int16	 = int16_t;
	using int32	 = int32_t;
	using int64	 = int64_t;

	using uint8	 = uint8_t;
	using uint16 = uint16_t;
	using uint32 = uint32_t;
	using uint64 = uint64_t;

	// 8. 便利マクロ / ヘルパー関数

	// メモリ安全解放 (ポインタ削除)
	template<typename T>
	inline void SafeDelete(T*& ptr) {
		if (ptr) {
			delete ptr;
			ptr = nullptr;
		}
	}
	#define SAFE_DELETE(x) Span::SafeDelete(x)

	// COMオブジェクト安全解放 (Release呼び出し)
	template<typename T>
	inline void SafeRelease(T*& ptr) {
		if (ptr) {
			ptr->Release();
			ptr = nullptr;
		}
	}
	#define SAFE_RELEASE(x) Span::SafeRelease(x)

	// コピー禁止マクロ (クラス定義内で使用)
	#define SPAN_NON_COPYABLE(ClassName) \
		ClassName(const ClassName&) = delete; \
		ClassName& operator=(const ClassName&) = delete;
}

// ログ出力
#include "Log/Logger.h"
