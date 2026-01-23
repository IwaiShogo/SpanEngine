#include "Core/CoreMinimal.h"
#include "Core/Time/Time.h"
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

class PlaygroundApp : public Span::Application
{
public:
	void OnStart() override
	{
		SPAN_LOG("Playground App Started!");

		// システム登録
		GetWorld()->AddSystem<SpinSystem>();
		GetWorld()->AddSystem<Span::CameraSystem>();
		GetWorld()->AddSystem<Span::RenderSystem>();

		ID3D12Device* device = GetRenderer().GetDevice();

		// --- アセット生成 ---
		static Span::Mesh* sphereMesh = Span::Mesh::CreateSphere(device, 64, 64); // 滑らかにするため分割数を倍増
		static Span::Mesh* planeMesh = Span::Mesh::CreatePlane(device, 100.0f, 100.0f);
		static Span::Mesh* cubeMesh = Span::Mesh::CreateCube(device);

		// 床 (市松模様っぽく見せるため、少し暗めの反射床に)
		Span::Material* matFloor = new Span::Material(); matFloor->Initialize(device);
		matFloor->SetAlbedo({ 0.1f, 0.1f, 0.15f }); // ダークブルーグレー
		matFloor->SetRoughness(0.2f); // 少し反射
		matFloor->SetMetallic(0.5f);

		auto floor = GetWorld()->CreateEntity<Span::Transform, Span::MeshRenderer>();
		GetWorld()->SetComponent(floor, Span::Transform{ {0, -5.0f, 0} });
		GetWorld()->SetComponent(floor, Span::MeshRenderer{ planeMesh, matFloor });

		// ========================================================
		// クリスタル・スパイラル (螺旋配置)
		// ========================================================
		int count = 60;			// 個数
		float radius = 4.0f;	// 半径
		float heightStep = 0.3f;// 1個ごとの高さ
		float angleStep = 0.5f; // 1個ごとの角度(ラジアン)

		for (int i = 0; i < count; ++i)
		{
			// 1. 座標計算 (螺旋)
			float angle = i * angleStep;
			float y = -4.0f + (i * heightStep);
			float x = cos(angle) * radius;
			float z = sin(angle) * radius;

			// 2. 色計算 (虹色グラデーション)
			// HSL -> RGB の簡易計算
			float hue = (float)i / count; // 0.0 ~ 1.0
			float r = fabsf(hue * 6.0f - 3.0f) - 1.0f;
			float g = 2.0f - fabsf(hue * 6.0f - 2.0f);
			float b = 2.0f - fabsf(hue * 6.0f - 4.0f);
			Span::Vector3 color = {
				std::clamp(r, 0.0f, 1.0f),
				std::clamp(g, 0.0f, 1.0f),
				std::clamp(b, 0.0f, 1.0f)
			};

			// 3. マテリアル作成 (ガラス)
			Span::Material* mat = new Span::Material();
			mat->Initialize(device);
			mat->SetAlbedo(color);
			mat->SetRoughness(0.0f); // 完全なツルツル
			mat->SetMetallic(0.1f);	 // 少しだけ金属感を入れて反射を強調
			mat->SetOpacity(0.4f);	 // 半透明！

			// 4. Entity生成
			auto ball = GetWorld()->CreateEntity<Span::Transform, Span::MeshRenderer>();
			GetWorld()->SetComponent(ball, Span::Transform{ {x, y, z}, {0,0,0}, {1.2f, 1.2f, 1.2f} });
			GetWorld()->SetComponent(ball, Span::MeshRenderer{ sphereMesh, mat });
		}

		// 中心に発光体っぽいコアを置く (不透明, Metallic MAX)
		Span::Material* matCore = new Span::Material(); matCore->Initialize(device);
		matCore->SetAlbedo({ 1.0f, 0.9f, 0.5f }); // 金色
		matCore->SetMetallic(1.0f);
		matCore->SetRoughness(0.1f);

		auto core = GetWorld()->CreateEntity<Span::Transform, Span::MeshRenderer, Spinner>();
		GetWorld()->SetComponent(core, Span::Transform{ {0, 0, 0}, {0,0,0}, {2.0f, 2.0f, 2.0f} });
		GetWorld()->SetComponent(core, Span::MeshRenderer{ cubeMesh, matCore });
		GetWorld()->SetComponent(core, Spinner{ 0.5f });

		// カメラ設定 (全体を見渡す位置)
		auto cam = GetWorld()->CreateEntity<Span::Transform, Span::Camera>();
		GetWorld()->SetComponent(cam, Span::Transform{ {0, 12.0f, -25.0f}, {15.0f, 0, 0} });
		GetWorld()->SetComponent(cam, Span::Camera{});
	}
};

Span::Application* Span::CreateApplication()
{
    return new PlaygroundApp();
}