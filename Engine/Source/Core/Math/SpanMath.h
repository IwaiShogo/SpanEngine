#pragma once
#include <DirectXMath.h>

namespace Span
{
	using namespace DirectX;

	// --- Vector2 ---
	struct Vector2
	{
		float x, y;

		Vector2() : x(0), y(0) {}
		Vector2(float _x, float _y) : x(_x), y(_y) {}

		// DirectXMathへの変換
		XMVECTOR ToXM() const { return XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat2(reinterpret_cast<XMFLOAT2*>(this), v); }

		// 足し算 (+)
		Vector2 operator+(const Vector2& other) const
		{
			Vector2 r;
			r.FromXM(XMVectorAdd(ToXM(), other.ToXM()));
			return r;
		}

		// 引き算 (-)
		Vector2 operator-(const Vector2& other) const
		{
			Vector2 r;
			r.FromXM(XMVectorSubtract(ToXM(), other.ToXM()));
			return r;
		}

		// スカラー掛け算 (*)
		Vector2 operator*(float scalar) const
		{
			Vector2 r;
			r.FromXM(XMVectorScale(ToXM(), scalar));
			return r;
		}
	};

	// --- Vector3 ---
	struct Vector3
	{
		float x, y, z;

		Vector3() : x(0), y(0), z(0) {}
		Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

		// DirectXMathへの変換ヘルパー
		XMVECTOR ToXM() const { return XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(this), v); }

		// 足し算 (+)
		Vector3 operator+(const Vector3& other) const
		{
			Vector3 r;
			r.FromXM(XMVectorAdd(ToXM(), other.ToXM()));
			return r;
		}

		// 引き算 (-)
		Vector3 operator-(const Vector3& other) const
		{
			Vector3 r;
			r.FromXM(XMVectorSubtract(ToXM(), other.ToXM()));
			return r;
		}

		// スカラー掛け算 (*)
		Vector3 operator*(float scalar) const
		{
			Vector3 r;
			r.FromXM(XMVectorScale(ToXM(), scalar));
			return r;
		}
	};

	// --- Matrix4x4 (4x4行列) ---
	// 3D空間での移動・回転・拡大縮小をひとまとめにした魔法の箱
	struct Matrix4x4
	{
		union
		{
			struct
			{
				float _11, _12, _13, _14;
				float _21, _22, _23, _24;
				float _31, _32, _33, _34;
				float _41, _42, _43, _44;
			};
			float m[4][4];
		};

		Matrix4x4() { Identity(); }

		XMMATRIX ToXM() const { return XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(this)); }
		void FromXM(XMMATRIX mat) { XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(this), mat); }

		// 単位行列 (何もしない行列) にリセット
		void Identity()
		{
			FromXM(XMMatrixIdentity());
		}

		// 行列の掛け算
		Matrix4x4 operator*(const Matrix4x4& other) const
		{
			Matrix4x4 result;
			result.FromXM(XMMatrixMultiply(ToXM(), other.ToXM()));
			return result;
		}
	};

	// 定数
	constexpr float PI = 3.1415926535f;

	// ラジアン変換
	inline float ToRadians(float degrees) { return degrees * (PI / 180.0f); }
}
