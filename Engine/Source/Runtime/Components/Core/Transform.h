/*****************************************************************//**
 * @file	Transform.h
 * @brief	位置、回転、拡大縮小を管理するコンポーネント。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/Math/SpanMath.h"
#include "Runtime/Reflection/SpanReflection.h"

namespace Span
{
	/**
	 * @struct	Transform
	 * @brief	📍 オブジェクトの3次元的な位置情報を保持するコンポーネント。
	 *
	 * @details
	 * 全てのオブジェクトの基本となるコンポーネントです。
	 * 階層構造 (親子関係) がある場合、この値は「親からの相対座標(Local)」として扱われます。
	 */
	struct Transform
	{
		Vector3 Position;		///< 位置 (Local)
		Quaternion Rotation;	///< 回転 (Local Quaternion)
		Vector3 Scale;			///< スケール (Local)

		// Constructors
		// ============================================================
		Transform()
			: Position(Vector3::Zero)
			, Rotation(Quaternion::Identity)
			, Scale(Vector3::One)
		{}

		Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
			: Position(position)
			, Rotation(rotation)
			, Scale(scale)
		{}

		explicit Transform(const Vector3& position)
			: Position(position)
			, Rotation(Quaternion::Identity)
			, Scale(Vector3::One)
		{}

		// Helpers
		// ============================================================

		/// @brief	単位行列 (初期値) を持つTransformを返します。
		static Transform Identity()
		{
			return Transform();
		}

		/// @brief	ローカル変換行列 (T * R * S) を計算して取得します。
		Matrix4x4 GetLocalMatrix() const
		{
			return Matrix4x4::TRS(Position, Rotation, Scale);
		}

		/// @brief	前方ベクトル (Local Forward / Z+) を取得します。
		Vector3 GetForward() const
		{
			// 回転クォータニオンによって (0,0,1) を回転させる
			Matrix4x4 mat = Matrix4x4::Rotation(Rotation);
			// 回転行列の3行目(Z軸)を取り出すのと同義
			return Vector3(mat._31, mat._32, mat._33).Normalized();
		}

		/// @brief	右方向ベクトル (Local Right / X+) を取得します。
		Vector3 GetRight() const
		{
			Matrix4x4 mat = Matrix4x4::Rotation(Rotation);
			return Vector3(mat._11, mat._12, mat._13).Normalized();
		}

		/// @brief	上方向ベクトル (Local Up / Y+) を取得します。
		Vector3 GetUp() const
		{
			Matrix4x4 mat = Matrix4x4::Rotation(Rotation);
			return Vector3(mat._21, mat._22, mat._23).Normalized();
		}

		/**
		 * @brief	指定したターゲット座標を向くように回転を設定します。
		 * @param	target 向きたいワールド座標
		 * @param	worldUp 世界の上方向 (通常は Y+)
		 */
		void LookAt(const Vector3& target, const Vector3& worldUp = Vector3::Up)
		{
			// 1. 方向ベクトル計算 (自分 -> ターゲット)
			Vector3 forward = (target - Position).Normalized();

			// ターゲットが近すぎて計算できない場合は何もしない
			if (Vector3::Dot(forward, forward) < 0.001f) return;

			// 2. 基底ベクトルの算出 (外積)
			Vector3 right = Vector3::Cross(worldUp, forward).Normalized();

			// 真上や真下を向いた場合の特異点対策
			if (Vector3::Dot(right, right) < 0.001f)
			{
				// worldUpとforwardが平行に近い -> Rightを適当に決める
				right = Vector3::Cross(Vector3::Right, forward).Normalized();
			}

			Vector3 up = Vector3::Cross(forward, right).Normalized();

			// 3. 回転行列の構築
			Matrix4x4 rotMat = Matrix4x4::Identity();

			// X軸 (Right)
			rotMat._11 = right.x; rotMat._12 = right.y; rotMat._13 = right.z;
			// Y軸 (Up)
			rotMat._21 = up.x;	  rotMat._22 = up.y;	rotMat._23 = up.z;
			// Z軸 (Forward)
			rotMat._31 = forward.x; rotMat._32 = forward.y; rotMat._33 = forward.z;

			// 4. 行列 -> クォータニオン変換して適用
			Rotation = Quaternion::FromRotationMatrix(rotMat);
		}

		// Reflection (Editor UI)
		// ============================================================
		SPAN_INSPECTOR_BEGIN(Transform)
			SPAN_FIELD(Position)
			SPAN_FIELD(Rotation)
			SPAN_FIELD(Scale)
		SPAN_INSPECTOR_END()
	};
}
