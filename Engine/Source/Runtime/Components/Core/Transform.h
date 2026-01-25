#pragma once
#include "Core/Math/SpanMath.h" 

namespace Span
{
	struct Transform
	{
		Vector3 Position;
		Quaternion Rotation;
		Vector3 Scale;

		// コンストラクタ
		Transform()
			: Position(Vector3::Zero)
			, Rotation(Quaternion::Identity)
			, Scale(Vector3::One)
		{
		}

		Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
			: Position(position)
			, Rotation(rotation)
			, Scale(scale)
		{
		}

		explicit Transform(const Vector3& position)
			: Position(position)
			, Rotation(Quaternion::Identity)
			, Scale(Vector3::One)
		{
		}

		// --- 静的作成ヘルパー ---

		static Transform Identity()
		{
			return Transform();
		}

		// --- 計算ヘルパー (自身のデータは変更せず、計算結果を返す) ---

		// ローカル行列 (T * R * S) を取得
		Matrix4x4 GetLocalMatrix() const
		{
			return Matrix4x4::TRS(Position, Rotation, Scale);
		}

		// --- 方向ベクトル取得 ---

		// 前方 (Z+)
		Vector3 GetForward() const
		{
			// 回転クォータニオンによって (0,0,1) を回転させる
			Matrix4x4 mat = Matrix4x4::Rotation(Rotation);
			// 回転行列の3行目(Z軸)を取り出すのと同義
			return Vector3(mat._31, mat._32, mat._33).Normalized();
		}

		// 上方 (Y+)
		Vector3 GetUp() const
		{
			Matrix4x4 mat = Matrix4x4::Rotation(Rotation);
			return Vector3(mat._21, mat._22, mat._23).Normalized();
		}

		// 右方 (X+)
		Vector3 GetRight() const
		{
			Matrix4x4 mat = Matrix4x4::Rotation(Rotation);
			return Vector3(mat._11, mat._12, mat._13).Normalized();
		}

		// --- 操作ヘルパー ---

		// 指定座標を向く回転を設定
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
	};
}