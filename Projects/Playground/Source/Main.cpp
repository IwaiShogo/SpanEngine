#include "Runtime/EntryPoint.h"
#include "Runtime/Application.h"
#include "Core/Log/Logger.h"
#include "Core/Math/SpanMath.h"

// --- Systems ---
#include "Runtime/Systems/Core/TransformSystem.h"
#include "Runtime/Systems/Graphics/CameraSystem.h"
#include "Runtime/Systems/Graphics/RenderingSystem.h"
#include "Runtime/Systems/Graphics/EditorCameraSystem.h"

// --- Components ---
#include "Runtime/Components/Core/Transform.h"
#include "Runtime/Components/Core/LocalToWorld.h"
#include "Runtime/Components/Core/Relationship.h" // ★追加
#include "Runtime/Components/Graphics/MeshFilter.h"
#include "Runtime/Components/Graphics/MeshRenderer.h"
#include "Runtime/Components/Graphics/Camera.h"
#include "Runtime/Components/Editor/EditorCamera.h"

// --- Resources ---
#include "Runtime/Graphics/Resources/Mesh.h"
#include "Runtime/Graphics/Resources/Material.h"
#include "Runtime/Graphics/Resources/Texture.h"

using namespace Span;

class PlaygroundApp : public Application
{
public:
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;

    Entity parentCube;
    Entity childSphere;

    void OnStart() override
    {
        SPAN_LOG("--- Playground App Started ---");

        GetWorld().AddSystem<EditorCameraSystem>();
        GetWorld().AddSystem<TransformSystem>();
        GetWorld().AddSystem<CameraSystem>();
        GetWorld().AddSystem<RenderingSystem>();

        ID3D12Device* device = GetRenderer().GetDevice();

        // --- リソース作成 ---
        {
            // 0: Plane
            meshes.push_back(Mesh::CreatePlane(device, 20.0f, 20.0f));

            // 1: Cube
            meshes.push_back(Mesh::CreateCube(device));

            // 2: Sphere (球体がないのでCubeで代用、あるいはModelLoaderがあればそれを使う)
            // 今回は親子関係が分かればいいのでCubeを使い回します
            meshes.push_back(Mesh::CreateCube(device));

            // Materials
            Material* gray = new Material(); gray->Initialize(device); gray->SetAlbedo(Vector3(0.5f, 0.5f, 0.5f));
            materials.push_back(gray);

            Material* red = new Material(); red->Initialize(device); red->SetAlbedo(Vector3(0.8f, 0.1f, 0.1f));
            materials.push_back(red);

            Material* blue = new Material(); blue->Initialize(device); blue->SetAlbedo(Vector3(0.1f, 0.1f, 0.8f));
            materials.push_back(blue);
        }

        // --- Camera ---
        {
            Entity camera = GetWorld().CreateEntity<Transform, LocalToWorld, Camera, EditorCamera>();
            Transform t;
            t.Position = Vector3(0.0f, 5.0f, -10.0f);
            t.LookAt(Vector3::Zero);
            GetWorld().SetComponent(camera, t);
            GetWorld().SetComponent(camera, Camera(60.0f));
            GetWorld().SetComponent(camera, EditorCamera{});
        }

        // --- Floor ---
        {
            Entity floor = GetWorld().CreateEntity<Transform, LocalToWorld, MeshFilter, MeshRenderer>();
            GetWorld().SetComponent(floor, Transform(Vector3(0, 0, 0)));
            GetWorld().SetComponent(floor, MeshFilter(meshes[0]));
            GetWorld().SetComponent(floor, MeshRenderer(materials[0]));
        }

        // --- Parent Object (Red Cube) ---
        {
            parentCube = GetWorld().CreateEntity<Transform, LocalToWorld, MeshFilter, MeshRenderer>();

            Transform t;
            t.Position = Vector3(0, 1.5f, 0);
            GetWorld().SetComponent(parentCube, t);

            GetWorld().SetComponent(parentCube, MeshFilter(meshes[1]));
            GetWorld().SetComponent(parentCube, MeshRenderer(materials[1]));
        }

        // --- Child Object (Blue Cube/Sphere) ---
        // ★ Parentコンポーネントを追加
        {
            childSphere = GetWorld().CreateEntity<Transform, LocalToWorld, MeshFilter, MeshRenderer, Parent>();

            // 親を設定
            GetWorld().SetComponent(childSphere, Parent(parentCube));

            Transform t;
            t.Position = Vector3(3.0f, 0.0f, 0.0f); // 親から右に3m離れた位置 (ローカル座標)
            t.Scale = Vector3(0.5f, 0.5f, 0.5f);    // 少し小さく
            GetWorld().SetComponent(childSphere, t);

            GetWorld().SetComponent(childSphere, MeshFilter(meshes[2]));
            GetWorld().SetComponent(childSphere, MeshRenderer(materials[2]));
        }
    }

    void OnUpdate() override
    {
        // 親をY軸回転させる
        if (GetWorld().IsAlive(parentCube))
        {
            Transform& t = GetWorld().GetComponent<Transform>(parentCube);
            t.Rotation = t.Rotation * Quaternion::AngleAxis(Vector3::Up, Time::GetDeltaTime() * 1.0f);
        }

        // 子をX軸回転させる（自転）
        // 親が回るので、子は「親の周りを公転しながら、自分で自転する」動きになるはず
        if (GetWorld().IsAlive(childSphere))
        {
            Transform& t = GetWorld().GetComponent<Transform>(childSphere);
            t.Rotation = t.Rotation * Quaternion::AngleAxis(Vector3::Right, Time::GetDeltaTime() * 2.0f);
        }
    }

    void OnShutdown() override
    {
        for (auto m : meshes) { m->Shutdown(); delete m; }
        for (auto m : materials) { m->Shutdown(); delete m; }
        meshes.clear();
        materials.clear();
    }
};

Application* Span::CreateApplication()
{
    return new PlaygroundApp();
}