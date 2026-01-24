#include "Core/CoreMinimal.h"
#include "Core/Time/Time.h"
#include "Core/Input/Input.h"
#include "Core/Math/SpanMath.h"
#include "Runtime/EntryPoint.h"
#include "Runtime/Application.h"

// ECS & Components
#include "Runtime/ECS/Kernel/System.h"
#include "Runtime/Components/Core/Transform.h"
#include "Runtime/Components/Graphics/GraphicsComponents.h"
#include "Runtime/Components/Graphics/Camera.h"

// Systems
#include "Runtime/Systems/Graphics/RenderSystem.h"
#include "Runtime/Systems/Graphics/CameraSystem.h"

#include "Runtime/Graphics/ModelLoader.h"
#include "Runtime/Graphics/Resources/Texture.h"

// 回転用コンポーネント
struct Spinner
{
	float speed;
};

// 回転システム
class SpinSystem : public Span::System
{
public:
	void OnUpdate() override
	{
		float dt = Span::Time::GetDeltaTime();
		GetWorld()->ForEach<Span::Transform, Spinner>(
			[dt](Span::Entity e, Span::Transform& tf, Spinner& spin)
			{
				tf.Rotation.y += spin.speed * dt * 20.0f;
			}
		);
	}
};

class CameraControllerSystem : public Span::System
{
public:
	void OnUpdate() override
	{
		float dt = Span::Time::GetDeltaTime();
		float moveSpeed = 5.0f;
		float rotateSpeed = 0.2f;

		// Shiftキーで加速
		if (Span::Input::GetKey(Span::Key::LeftShift)) moveSpeed *= 3.0f;

		GetWorld()->ForEach<Span::Transform, Span::Camera>(
			[&](Span::Entity e, Span::Transform& tf, Span::Camera& cam)
			{
				// --- 回転 (右クリック中のみ) ---
				if (Span::Input::GetKey(Span::Key::MouseRight))
				{
					Span::Vector2 mouseDelta = Span::Input::GetMouseDelta();
					tf.Rotation.y += mouseDelta.x * rotateSpeed;
					tf.Rotation.x += mouseDelta.y * rotateSpeed;
				}

				// --- 移動 (WASD) ---
				// 現在の回転に合わせて移動ベクトルを計算
				Span::Matrix4x4 rotMat;
				// Y軸回転のみ考慮して移動（FPSスタイル）
				rotMat.FromXM(XMMatrixRotationY(Span::ToRadians(tf.Rotation.y)));

				Span::Vector3 forward = { 0, 0, 1 };
				Span::Vector3 right = { 1, 0, 0 };

				// 行列でベクトルを回転させる簡易実装（本来はVector3クラスに機能を持たせるべき）
				auto TransformVec = [&](Span::Vector3 v) {
					XMVECTOR vec = v.ToXM();
					vec = XMVector3TransformNormal(vec, rotMat.ToXM());
					Span::Vector3 res; res.FromXM(vec); return res;
					};

				forward = TransformVec(forward);
				right = TransformVec(right);

				if (Span::Input::GetKey(Span::Key::W)) tf.Position = tf.Position + forward * (moveSpeed * dt);
				if (Span::Input::GetKey(Span::Key::S)) tf.Position = tf.Position - forward * (moveSpeed * dt);
				if (Span::Input::GetKey(Span::Key::D)) tf.Position = tf.Position + right * (moveSpeed * dt);
				if (Span::Input::GetKey(Span::Key::A)) tf.Position = tf.Position - right * (moveSpeed * dt);

				// 上昇・下降 (E/Q)
				if (Span::Input::GetKey(Span::Key::E)) tf.Position.y += moveSpeed * dt;
				if (Span::Input::GetKey(Span::Key::Q)) tf.Position.y -= moveSpeed * dt;
			}
		);
	}
};

class PlaygroundApp : public Span::Application
{
public:
	void OnStart() override
	{
		SPAN_LOG("Playground App Started!");

		GetWorld()->AddSystem<SpinSystem>();
		GetWorld()->AddSystem<CameraControllerSystem>();
		GetWorld()->AddSystem<Span::CameraSystem>();
		GetWorld()->AddSystem<Span::RenderSystem>();

		ID3D12Device* device = GetRenderer().GetDevice();
		auto queue = GetRenderer().GetCommandQueue(); // ★コマンドキューが必要

		// --- テクスチャ読み込み ---
		// ※ Projects/Playground フォルダに "test.jpg" を置いてください
		static Span::Texture* texture = new Span::Texture();
		bool texLoaded = texture->Initialize(device, queue, "Assets/test.jpg"); // ファイル名は適宜変更

		if (!texLoaded) {
			SPAN_WARN("Texture not found! Using default white color.");
		}

		// --- メッシュ ---
		static Span::Mesh* sphereMesh = Span::Mesh::CreateSphere(device, 32, 32);
		static Span::Mesh* capsuleMesh = Span::Mesh::CreateCapsule(device); // さっき作ったカプセル！

		// --- マテリアル ---
		Span::Material* matTex = new Span::Material();
		matTex->Initialize(device);
		matTex->SetAlbedo({ 1.0f, 1.0f, 1.0f }); // 白 (画像の色をそのまま出すため)
		matTex->SetRoughness(0.8f);

		// ★テクスチャをセット！
		if (texLoaded) {
			matTex->SetTexture(texture);
		}

		// Entity配置: カプセル
		auto capsule = GetWorld()->CreateEntity<Span::Transform, Span::MeshRenderer, Spinner>();
		GetWorld()->SetComponent(capsule, Span::Transform{ {0, 0, 0}, {0,0,0}, {1.5f, 1.5f, 1.5f} });
		GetWorld()->SetComponent(capsule, Span::MeshRenderer{ capsuleMesh, matTex });
		GetWorld()->SetComponent(capsule, Spinner{ 1.0f });

		// カメラ
		auto cam = GetWorld()->CreateEntity<Span::Transform, Span::Camera>();
		GetWorld()->SetComponent(cam, Span::Transform{ {0, 2.0f, -6.0f} });
		GetWorld()->SetComponent(cam, Span::Camera{});
	}
};

Span::Application* Span::CreateApplication()
{
    return new PlaygroundApp();
}