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

	// --- 定数とヘルパー関数 ---
	constexpr float PI = 3.1415926535f;
	constexpr float TwoPI = PI * 2.0f;
	constexpr float HalfPI = PI / 2.0f;

	inline float ToRadians(float degrees) { return degrees * (PI / 180.0f); }
	inline float ToDegrees(float radians) { return radians * (180.0f / PI); }

	template<typename T>
	inline T Clamp(T value, T min, T max) { return (value < min) ? min : ((value > max) ? max : value); }

	template<typename T>
	inline T Lerp(T a, T b, float t) { return a + (b - a) * t; }

	// --- Vector2 ---
	struct Vector2
	{
		float x, y;

		Vector2() : x(0), y(0) {}
		Vector2(float _x, float _y) : x(_x), y(_y) {}
		explicit Vector2(float v) : x(v), y(v) {}

		// DirectXMath 変換
		XMVECTOR ToXM() const { return XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat2(reinterpret_cast<XMFLOAT2*>(this), v); }

		// 演算子
		Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
		Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
		Vector2 operator*(float s) const { return Vector2(x * s, y * s); }
		Vector2 operator/(float s) const { return Vector2(x / s, y / s); }
		Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
		Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }

		// 機能
		float Length() const { return std::sqrt(x * x + y * y); }
		float LengthSquared() const { return x * x + y * y; }
		Vector2 Normalized() const
		{
			float len = Length();
			return (len > 0) ? Vector2(x / len, y / len) : Vector2(0, 0);
		}

		static const Vector2 Zero;
		static const Vector2 One;
	};

	// --- Vector3 ---
	struct Vector3
	{
		float x, y, z;

		Vector3() : x(0), y(0), z(0) {}
		Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
		explicit Vector3(float v) : x(v), y(v), z(v) {}

		// DirectXMath 変換
		XMVECTOR ToXM() const { return XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(this), v); }

		// 演算子
		Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
		Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
		Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
		Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }
		Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
		Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }

		// 外積・内積
		static float Dot(const Vector3& a, const Vector3& b)
		{
			return XMVectorGetX(XMVector3Dot(a.ToXM(), b.ToXM()));
		}

		static Vector3 Cross(const Vector3& a, const Vector3& b)
		{
			Vector3 r; r.FromXM(XMVector3Cross(a.ToXM(), b.ToXM())); return r;
		}

		// 機能
		float Length() const { return std::sqrt(x * x + y * y + z * z); }

		Vector3 Normalized() const
		{
			Vector3 r; r.FromXM(XMVector3Normalize(ToXM())); return r;
		}

		static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
		{
			Vector3 r; r.FromXM(XMVectorLerp(a.ToXM(), b.ToXM(), t)); return r;
		}

		// 定数 (Unityライクな座標系: Y-Up, Z-Forward)
		static const Vector3 Zero;
		static const Vector3 One;
		static const Vector3 Up;	  // (0, 1, 0)
		static const Vector3 Down;	  // (0, -1, 0)
		static const Vector3 Forward; // (0, 0, 1)
		static const Vector3 Back;	  // (0, 0, -1)
		static const Vector3 Right;	  // (1, 0, 0)
		static const Vector3 Left;	  // (-1, 0, 0)
	};

	// --- Vector4 ---
	struct Vector4
	{
		float x, y, z, w;

		Vector4() : x(0), y(0), z(0), w(0) {}
		Vector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
		Vector4(const Vector3& v, float _w) : x(v.x), y(v.y), z(v.z), w(_w) {}

		XMVECTOR ToXM() const { return XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(this), v); }

		Vector4 operator*(float s) const { return Vector4(x * s, y * s, z * s, w * s); }
	};

	// --- Quaternion (回転) ---
	struct Quaternion
	{
		float x, y, z, w;

		Quaternion() : x(0), y(0), z(0), w(1) {} // Identity
		Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

		XMVECTOR ToXM() const { return XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(this)); }
		void FromXM(XMVECTOR v) { XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(this), v); }

		// Euler角(Pitch, Yaw, Roll)から作成 (ラジアン)
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

		// 現在のQuaternionをEuler角 (Pitch, Yaw, Roll) に変換
		Vector3 ToEuler() const;

		static Quaternion AngleAxis(const Vector3& axis, float angle)
		{
			Quaternion q;
			q.FromXM(XMQuaternionRotationAxis(axis.ToXM(), angle));
			return q;
		}

		// 回転行列からクォータニオンを作成
		static Quaternion FromRotationMatrix(const Matrix4x4& m);

		// 回転の掛け合わせ
		Quaternion operator*(const Quaternion& other) const
		{
			Quaternion r;
			r.FromXM(XMQuaternionMultiply(ToXM(), other.ToXM()));
			return r;
		}

		// 球面線形補間 (Slerp) - アニメーション用
		static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)
		{
			Quaternion r;
			r.FromXM(XMQuaternionSlerp(a.ToXM(), b.ToXM(), t));
			return r;
		}

		static const Quaternion Identity;
	};

	// --- Matrix4x4 ---
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

		Matrix4x4() { FromXM(XMMatrixIdentity()); }
		Matrix4x4(const Matrix4x4&) = default;

		XMMATRIX ToXM() const { return XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(this)); }
		void FromXM(XMMATRIX mat) { XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(this), mat); }

		// --- 静的作成メソッド (これらを代入に使用します) ---

		// 単位行列
		static Matrix4x4 Identity()
		{
			return Matrix4x4();
		}

		// 移動行列
		static Matrix4x4 Translation(const Vector3& position)
		{
			Matrix4x4 r; r.FromXM(XMMatrixTranslation(position.x, position.y, position.z)); return r;
		}

		// 回転行列 (Quaternionから)
		static Matrix4x4 Rotation(const Quaternion& rotation)
		{
			Matrix4x4 r; r.FromXM(XMMatrixRotationQuaternion(rotation.ToXM())); return r;
		}

		// 拡大縮小行列
		static Matrix4x4 Scale(const Vector3& scale)
		{
			Matrix4x4 r; r.FromXM(XMMatrixScaling(scale.x, scale.y, scale.z)); return r;
		}

		// TRS (Translation * Rotation * Scale)
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

		// ビュー行列作成 (左手系: Z+が奥)
		static Matrix4x4 LookAtLH(const Vector3& eye, const Vector3& focus, const Vector3& up)
		{
			Matrix4x4 r;
			r.FromXM(XMMatrixLookAtLH(eye.ToXM(), focus.ToXM(), up.ToXM()));
			return r;
		}

		// 透視投影行列作成 (左手系)
		static Matrix4x4 PerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ)
		{
			Matrix4x4 r;
			r.FromXM(XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ));
			return r;
		}

		// --- メンバ関数 ---

		// 逆行列
		Matrix4x4 Invert() const
		{
			Matrix4x4 r;
			XMVECTOR det;
			r.FromXM(XMMatrixInverse(&det, ToXM()));
			return r;
		}

		// 転置行列
		Matrix4x4 Transpose() const
		{
			Matrix4x4 r; r.FromXM(XMMatrixTranspose(ToXM())); return r;
		}

		// --- 演算子 ---

		// 行列同士の掛け算
		Matrix4x4 operator*(const Matrix4x4& other) const
		{
			Matrix4x4 r;
			r.FromXM(XMMatrixMultiply(ToXM(), other.ToXM()));
			return r;
		}
	};

	// 遅延実装
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

	// 静的メンバの実体定義 (ヘッダ内記述のため inline を使用)
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