/*****************************************************************//**
 * @file	SpanMath.h
 * @brief	算術ライブラリ (Vector, Matrix, Quaternion)。
 *
 * @details
 * DirectXMath (SIMD) をラップ氏、使いやすい構造体として提供します。
 *
 * ### 📏 座標系と仕様 (Coordinate System)
 * - **座標系**: 左手座標系 (Left-Handed)
 * - +X: Right, +Y: Up, +Z: Forward (奥)
 * - **回転**: 時計回りが正 (Locking along the axis)
 * - **行列**: 行優先 (Row-Major) ストレージ
 * - **単位**: 角度は原則として **ラジアン (Radians)** を使用
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

namespace Span
{
	using namespace DirectX;

	// 前方宣言
	struct Vector2;
	struct Vector3;
	struct Vector4;
	struct Quaternion;
	struct Matrix4x4;

	/**
	 * @name	Constants & Helpers
	 * 定数及びヘルパー関数
	 */
	/// @{

	constexpr float PI     = 3.1415926535f;
	constexpr float TwoPI  = PI * 2.0f;
	constexpr float HalfPI = PI / 2.0f;

	/// @brief	度数法(Degree) を 弧度法(Radian) に変換
	inline float ToRadians(float degrees) { return degrees * (PI / 180.0f); }

	/// @brief	弧度法(Radian) を 度数法(Degree) に変換
	inline float ToDegrees(float radians) { return radians * (180.0f / PI); }

	/// @brief	値を指定範囲に制限します
	template<typename T>
	inline T Clamp(T value, T min, T max) { return (value < min) ? min : ((value > max) ? max : value); }

	/// @brief	線形補間 (Linear Interpolation)
	template<typename T>
	inline T Lerp(T a, T b, float t) { return a + (b - a) * t; }

	/// @}

	/**
	 * @struct	Vector2
	 * @brief	2次元ベクトル (float x, y)
	 * UI座標やUV座標に使用されます。
	 */
	struct Vector2
	{
		float x, y;

		// ---- Constructors ---
		Vector2() : x(0), y(0) {}
		Vector2(float _x, float _y) : x(_x), y(_y) {}
		explicit Vector2(float v) : x(v), y(v) {}

		/// @name	DirectXMath Interop
		/// @{
		XMVECTOR ToXM() const { return XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat2(reinterpret_cast<XMFLOAT2*>(this), v); }
		/// @}

		/// @name	Operators
		/// @{
		Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
		Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
		Vector2 operator*(float s) const { return Vector2(x * s, y * s); }
		Vector2 operator/(float s) const { return Vector2(x / s, y / s); }
		Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
		Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
		/// @}

		/// @name	Utilities
		/// @{
		float Length() const { return std::sqrt(x * x + y * y); }
		float LengthSquared() const { return x * x + y * y; }

		/// @brief	正規化 (長さ1) されたベクトルを返します。長さが0の場合はゼロベクトルを返します。
		Vector2 Normalized() const
		{
			float len = Length();
			return (len > 0) ? Vector2(x / len, y / len) : Vector2(0, 0);
		}
		/// @}

		static const Vector2 Zero;
		static const Vector2 One;
	};

	/**
	 * @struct	Vector3
	 * @brief	3次元ベクトル (float x, y, z)
	 * 位置、方向、スケールなどに使用されます。
	 */
	struct Vector3
	{
		float x, y, z;

		// --- Constructors ---
		Vector3() : x(0), y(0), z(0) {}
		Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
		explicit Vector3(float v) : x(v), y(v), z(v) {}

		/// @name	DirectXMath Interop
		/// @{
		XMVECTOR ToXM() const { return XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(this), v); }
		/// @}

		/// @name	Operators
		/// @{
		Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
		Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
		Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
		Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }
		Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
		Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
		Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
		Vector3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }
		/// @}

		/// @name	Utility
		/// @{

		/// @brief	内積 (Dot Product)
		/// @return a・b (スカラ値)
		static float Dot(const Vector3& a, const Vector3& b)
		{
			return XMVectorGetX(XMVector3Dot(a.ToXM(), b.ToXM()));
		}

		/// @brief	外積 (Cross Product)
		/// @return a x b (ベクトル)
		static Vector3 Cross(const Vector3& a, const Vector3& b)
		{
			Vector3 r; r.FromXM(XMVector3Cross(a.ToXM(), b.ToXM())); return r;
		}

		/// @brief	ベクトルの長さ (Magnitude)
		float Length() const { return std::sqrt(x * x + y * y + z * z); }

		/// @brief		ベクトルの長さの二乗 (Squared Magnitude)
		/// @details	平方根計算を行わないため高速。距離比較などに使用。
		float LengthSquared() const
		{
			return x * x + y * y + z * z;
		}

		/// @brief	正規化ベクトルを返します (SIMD高速化版)
		Vector3 Normalized() const
		{
			Vector3 r; r.FromXM(XMVector3Normalize(ToXM())); return r;
		}

		static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
		{
			Vector3 r; r.FromXM(XMVectorLerp(a.ToXM(), b.ToXM(), t)); return r;
		}
		/// @}

		// --- Constants ---
		static const Vector3 Zero;
		static const Vector3 One;
		static const Vector3 Up;	  ///< (0, 1, 0)
		static const Vector3 Down;	  ///< (0, -1, 0)
		static const Vector3 Forward; ///< (0, 0, 1)
		static const Vector3 Back;	  ///< (0, 0, -1)
		static const Vector3 Right;	  ///< (1, 0, 0)
		static const Vector3 Left;	  ///< (-1, 0, 0)
	};

	/**
	 * @struct	Vector4
	 * @brief	4次元ベクトル (float x, y, z, w)
	 * 色情報(RGBA)や同次座標で使用されます。
	 */
	struct Vector4
	{
		float x, y, z, w;

		// --- Constructors ---
		Vector4() : x(0), y(0), z(0), w(0) {}
		Vector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
		Vector4(const Vector3& v, float _w) : x(v.x), y(v.y), z(v.z), w(_w) {}

		/// @name	DirectXMath Interop
		/// @{
		XMVECTOR ToXM() const { return XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(this), v); }
		/// @}

		/// @name	Operators
		Vector4 operator*(float s) const { return Vector4(x * s, y * s, z * s, w * s); }
		/// @}
	};

	/**
	 * @struct	Quaternion
	 * @brief	回転を表すクォータニオン (x, y, z, w)
	 * ジンバルロックを防ぎ、滑らかな補間(Slerp)を可能にします。
	 */
	struct Quaternion
	{
		float x, y, z, w;

		// --- Constructors ---
		Quaternion() : x(0), y(0), z(0), w(1) {} // Identity
		Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

		/// @name	DirectXMath Interop
		/// @{
		XMVECTOR ToXM() const { return XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(this), v); }
		/// @}

		/// @name	Creators
		/// @{

		/**
		 * @brief	オイラー角から作成
		 * @param	pitch	X軸回転 (ラジアン)
		 * @param	yaw		Y軸回転 (ラジアン)
		 * @param	roll	Z軸回転 (ラジアン)
		 */
		static Quaternion FromEuler(float pitch, float yaw, float roll)
		{
			Quaternion q;
			q.FromXM(XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
			return q;
		}

		static Quaternion FromEuler(const Vector3& euler)
		{
			return FromEuler(euler.x, euler.y, euler.z);
		}

		/// @brief	軸と角度から作成
		static Quaternion AngleAxis(const Vector3& axis, float angle)
		{
			Quaternion q;
			q.FromXM(XMQuaternionRotationAxis(axis.ToXM(), angle));
			return q;
		}

		/// @brief	回転行列から変換 (実装は後述)
		static Quaternion FromRotationMatrix(const Matrix4x4& m);
		/// @}

		/**
		 * @brief	オイラー角 (Pitch, Yaw, Roll) に変換して返します
		 * @return	Vector3 (単位: ラジアン)
		 */
		Vector3 ToEuler() const;

		// 回転の合成 (q1 * q2)
		Quaternion operator*(const Quaternion& other) const
		{
			Quaternion r;
			r.FromXM(XMQuaternionMultiply(ToXM(), other.ToXM()));
			return r;
		}

		/// @brief	球面線形補間 (Slerp)
		static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)
		{
			Quaternion r;
			r.FromXM(XMQuaternionSlerp(a.ToXM(), b.ToXM(), t));
			return r;
		}

		static const Quaternion Identity;
	};

	/**
	 * @struct	Matrix4x4
	 * @brief	4x4 行列 (Row-Major)
	 *
	 * @details
	 * メモリ上では行優先(Row-Major)で格納されます。
	 * `_11, _12, _13, _14` が1行目の要素です。
	 * 変換の適用順序は `Vector * Matrix` ではなく `Matrix * Matrix` で合成し、
	 * シェーダーに送る際は転置(Transpose)して裂優先に変換する必要があります。
	 */
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

		// --- Constructors ---
		Matrix4x4() { FromXM(XMMatrixIdentity()); }
		Matrix4x4(const Matrix4x4&) = default;

		/// @name	DirectXMath Interop
		/// @{
		XMMATRIX ToXM() const { return XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(this)); }
		void FromXM(XMMATRIX mat) { XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(this), mat); }
		/// @}

		/// @name	Static Creators
		/// @{

		/// @brief	単位行列
		static Matrix4x4 Identity()
		{
			return Matrix4x4();
		}

		/// @brief	移動行列
		static Matrix4x4 Translation(const Vector3& position)
		{
			Matrix4x4 r; r.FromXM(XMMatrixTranslation(position.x, position.y, position.z)); return r;
		}

		/// @brief	回転行列
		static Matrix4x4 Rotation(const Quaternion& rotation)
		{
			Matrix4x4 r; r.FromXM(XMMatrixRotationQuaternion(rotation.ToXM())); return r;
		}

		/// @brief	拡大縮小行列
		static Matrix4x4 Scale(const Vector3& scale)
		{
			Matrix4x4 r; r.FromXM(XMMatrixScaling(scale.x, scale.y, scale.z)); return r;
		}

		/**
		 * @brief	TRS行列 (Translation * Rotation * Scale) を作成
		 * @note	適用順序: Scale -> Rotation -> Translation
		 */
		static Matrix4x4 TRS(const Vector3& t, const Quaternion& r, const Vector3& s)
		{
			Matrix4x4 mat;
			// Scale -> Rotate -> Translate の順 (DirectXMathは行優先なので S * R * T)
			XMMATRIX ms = XMMatrixScaling(s.x, s.y, s.z);
			XMMATRIX mr = XMMatrixRotationQuaternion(r.ToXM());
			XMMATRIX mt = XMMatrixTranslation(t.x, t.y, t.z);
			mat.FromXM(XMMatrixMultiply(XMMatrixMultiply(ms, mr), mt));
			return mat;
		}

		/// @brief	ビュー行列作成 (左手系: Z+が奥)
		static Matrix4x4 LookAtLH(const Vector3& eye, const Vector3& focus, const Vector3& up)
		{
			Matrix4x4 r;
			r.FromXM(XMMatrixLookAtLH(eye.ToXM(), focus.ToXM(), up.ToXM()));
			return r;
		}

		/// @brief	透視投影行列作成 (左手系)
		static Matrix4x4 PerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ)
		{
			Matrix4x4 r;
			r.FromXM(XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ));
			return r;
		}
		/// @}

		/// @name	Operations
		/// @{
		Matrix4x4 Invert() const
		{
			Matrix4x4 r;
			XMVECTOR det;
			r.FromXM(XMMatrixInverse(&det, ToXM()));
			return r;
		}

		/// @brief	転置行列
		Matrix4x4 Transpose() const
		{
			Matrix4x4 result;
			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
				{
					result.m[r][c] = m[c][r];
				}
			}
			return result;
		}

		/// @brief	行列を 移動・回転・スケール に分解する
		bool Decompose(Vector3& outTranslation, Quaternion& outRotation, Vector3& outScale) const
		{
			// 1. Translation
			outTranslation = Vector3(m[3][0], m[3][1], m[3][2]);

			// 2. Scale
			// 各軸の長さ(ノルム)を計算
			Vector3 xaxis(m[0][0], m[0][1], m[0][2]);
			Vector3 yaxis(m[1][0], m[1][1], m[1][2]);
			Vector3 zaxis(m[2][0], m[2][1], m[2][2]);

			outScale.x = xaxis.Length();
			outScale.y = yaxis.Length();
			outScale.z = zaxis.Length();

			if (outScale.x < 0.0001f || outScale.y < 0.0001f || outScale.z < 0.0001f)
				return false;

			// 回転行列の抽出のため正規化
			xaxis /= outScale.x;
			yaxis /= outScale.y;
			zaxis /= outScale.z;

			// 回転行列の構築
			Matrix4x4 rotationMtx = Identity();
			rotationMtx.m[0][0] = xaxis.x; rotationMtx.m[0][1] = xaxis.y; rotationMtx.m[0][2] = xaxis.z;
			rotationMtx.m[1][0] = yaxis.x; rotationMtx.m[1][1] = yaxis.y; rotationMtx.m[1][2] = yaxis.z;
			rotationMtx.m[2][0] = zaxis.x; rotationMtx.m[2][1] = zaxis.y; rotationMtx.m[2][2] = zaxis.z;

			// Quaternionへの変換
			outRotation = Quaternion::FromRotationMatrix(rotationMtx);

			return true;
		}

		/// @brief	行列同士の掛け算
		Matrix4x4 operator*(const Matrix4x4& other) const
		{
			Matrix4x4 r;
			r.FromXM(XMMatrixMultiply(ToXM(), other.ToXM()));
			return r;
		}
		/// @}
	};

	// --- Inline Implementations ---

	inline Quaternion Quaternion::FromRotationMatrix(const Matrix4x4& m)
	{
		Quaternion q;
		q.FromXM(XMQuaternionRotationMatrix(m.ToXM()));
		return q;
	}

	inline Vector3 Quaternion::ToEuler() const
	{
		// Matrixを経由して正確にEuler角を取り出す
		Matrix4x4 m = Matrix4x4::Rotation(*this);

		float pitch, yaw, roll;

		// Pitch (X軸回転)
		if (m._32 < -0.999f) pitch = HalfPI;
		else if (m._32 > 0.999f) pitch = -HalfPI;
		else pitch = asin(-m._32);

		// Yaw (Y軸回転) & Roll (Z軸回転)
		if (abs(m._32) < 0.999f)
		{
			yaw = atan2(m._31, m._33);
			roll = atan2(m._12, m._22);
		}
		else
		{
			yaw = atan2(-m._13, m._11);
			roll = 0.0f;
		}

		return Vector3(pitch, yaw, roll);
	}

	// 静的メンバ定義 (inline)
	inline const Vector2 Vector2::Zero(0, 0);
	inline const Vector2 Vector2::One(1, 1);

	inline const Vector3 Vector3::Zero(0, 0, 0);
	inline const Vector3 Vector3::One(1, 1, 1);
	inline const Vector3 Vector3::Up(0, 1, 0);
	inline const Vector3 Vector3::Down(0, -1, 0);
	inline const Vector3 Vector3::Forward(0, 0, 1);
	inline const Vector3 Vector3::Back(0, 0, -1);
	inline const Vector3 Vector3::Right(1, 0, 0);
	inline const Vector3 Vector3::Left(-1, 0, 0);

	inline const Quaternion Quaternion::Identity(0, 0, 0, 1);
}
