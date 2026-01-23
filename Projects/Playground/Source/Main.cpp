#include "Core/CoreMinimal.h"
#include "Core/Time/Time.h"
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

// ユーザー定義: 回転コンポーネント
struct Spinner
{
    float speed;
};

// ユーザー定義: 回転システム
class SpinSystem : public Span::System
{
public:
    void OnUpdate() override
    {
        float dt = Span::Time::GetDeltaTime();
        GetWorld()->ForEach<Span::Transform, Spinner>(
            [dt](Span::Entity e, Span::Transform& tf, Spinner& spin)
            {
                // Y軸で回転させる
                tf.Rotation.y += spin.speed * dt * 50.0f;
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

		// 1. システム登録
		GetWorld()->AddSystem<SpinSystem>();
		GetWorld()->AddSystem<Span::CameraSystem>();
		GetWorld()->AddSystem<Span::RenderSystem>();

		ID3D12Device* device = GetRenderer().GetDevice();

		// --- メッシュ生成 ---
		static Span::Mesh* planeMesh = Span::Mesh::CreatePlane(device, 20.0f, 20.0f);
		static Span::Mesh* cubeMesh = Span::Mesh::CreateCube(device);
		static Span::Mesh* sphereMesh = Span::Mesh::CreateSphere(device);
		static Span::Mesh* cylinderMesh = Span::Mesh::CreateCylinder(device);
		static Span::Mesh* coneMesh = Span::Mesh::CreateCone(device);
		static Span::Mesh* torusMesh = Span::Mesh::CreateTorus(device);
		static Span::Mesh* capsuleMesh = Span::Mesh::CreateCapsule(device, 0.5f, 2.0f);

		// --- マテリアル生成 ---
		static Span::Material* matWhite = new Span::Material(); matWhite->Initialize(device);
		static Span::Material* matRed = new Span::Material(); matRed->Initialize(device);	  matRed->SetAlbedo({ 1,0,0 });
		static Span::Material* matGreen = new Span::Material(); matGreen->Initialize(device); matGreen->SetAlbedo({ 0,1,0 });
		static Span::Material* matBlue = new Span::Material(); matBlue->Initialize(device);  matBlue->SetAlbedo({ 0,0,1 });
		static Span::Material* matGold = new Span::Material(); matGold->Initialize(device);  matGold->SetAlbedo({ 1,0.8f,0 }); matGold->SetMetallic(1.0f);

		// --- Entity配置 ---

		// 1. 地面
		auto floor = GetWorld()->CreateEntity<Span::Transform, Span::MeshRenderer>();
		GetWorld()->SetComponent(floor, Span::Transform{ {0, -1.0f, 0} });
		GetWorld()->SetComponent(floor, Span::MeshRenderer{ planeMesh, matWhite });

		// 2. 形状パレード
		struct ShapeItem { Span::Mesh* m; Span::Material* mat; float x; };
		std::vector<ShapeItem> items = {
			{ cubeMesh, matRed, -4.0f },
			{ sphereMesh, matGold, -2.0f },
			{ cylinderMesh, matGreen, 0.0f },
			{ coneMesh, matBlue, 2.0f },
			{ torusMesh, matRed, 4.0f },
			{ capsuleMesh, matGreen, 6.0f }
		};

		for (auto& item : items)
		{
			auto e = GetWorld()->CreateEntity<Span::Transform, Span::MeshRenderer, Spinner>();
			GetWorld()->SetComponent(e, Span::Transform{ {item.x, 0, 0} });
			GetWorld()->SetComponent(e, Span::MeshRenderer{ item.m, item.mat });
			GetWorld()->SetComponent(e, Spinner{ 1.0f });
		}

		// カメラ調整
		auto cam = GetWorld()->CreateEntity<Span::Transform, Span::Camera>();
		GetWorld()->SetComponent(cam, Span::Transform{ {0, 3, -15}, {15, 0, 0} }); // 少し上から見下ろす
		GetWorld()->SetComponent(cam, Span::Camera{});
    }
};

Span::Application* Span::CreateApplication()
{
    return new PlaygroundApp();
}