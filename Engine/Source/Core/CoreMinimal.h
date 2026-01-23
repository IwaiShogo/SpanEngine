#pragma once

// 1. Windows API 設定
// マクロの衝突を防ぐための定義
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

// 2. 標準ライブラリ (Standard Library)
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
#include <typeindex> // ECSの型ID用
#include <filesystem> // モデルローダー用

// 3. DirectX 12 & WRL (Windows Runtime Library)
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <DirectXMath.h>

// 4. 共通の名前空間の省略
using namespace Microsoft::WRL; // ComPtr用
using namespace DirectX;		// XMMATRIX, XMFLOAT3等用

namespace Span
{
	// 5. 基本型のエイリアス (RustやC#風の型定義)
	using int8	 = int8_t;
	using int16	 = int16_t;
	using int32	 = int32_t;
	using int64	 = int64_t;

	using uint8	 = uint8_t;
	using uint16 = uint16_t;
	using uint32 = uint32_t;
	using uint64 = uint64_t;

	// 6. 便利マクロ / ヘルパー関数

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