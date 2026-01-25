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
#include "Runtime/Components/Graphics/MeshFilter.h"
#include "Runtime/Components/Graphics/MeshRenderer.h"
#include "Runtime/Components/Graphics/Camera.h"

// --- Resources ---
#include "Runtime/Graphics/Resources/Mesh.h"
#include "Runtime/Graphics/Resources/Material.h"
#include "Runtime/Graphics/Resources/Texture.h"

using namespace Span;

class PlaygroundApp : public Application
{
public:
    // リソースの所有権管理
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;

    // 動かす対象のEntity ID
    Entity cubeEntity;

    void OnStart() override
    {
        SPAN_LOG("--- Playground App Started ---");

        // 1. システムの登録 (GetWorld()を使用)
        GetWorld().AddSystem<EditorCameraSystem>();
        GetWorld().AddSystem<TransformSystem>();
        GetWorld().AddSystem<CameraSystem>();
        GetWorld().AddSystem<RenderingSystem>();

        // デバイス取得
        ID3D12Device* device = GetRenderer().GetDevice();

        // 2. リソースの作成
        {
            // 床
            Mesh* planeMesh = Mesh::CreatePlane(device, 20.0f, 20.0f);
            Material* grayMat = new Material();
            grayMat->Initialize(device);
            grayMat->SetAlbedo(Vector3(0.5f, 0.5f, 0.5f));
            grayMat->SetRoughness(0.8f);

            meshes.push_back(planeMesh);
            materials.push_back(grayMat);

            // キューブ
            Mesh* cubeMesh = Mesh::CreateCube(device);
            Material* redMat = new Material();
            redMat->Initialize(device);
            redMat->SetAlbedo(Vector3(0.8f, 0.1f, 0.1f));
            redMat->SetRoughness(0.4f);
            redMat->SetMetallic(0.1f);

            meshes.push_back(cubeMesh);
            materials.push_back(redMat);
        }

        // 3. エンティティの作成

        // --- Camera Entity ---
        {
            Entity camera = GetWorld().CreateEntity<Transform, LocalToWorld, Camera>();

            Transform camTrans;
            camTrans.Position = Vector3(0.0f, 3.0f, 6.0f);
            camTrans.LookAt(Vector3(0.0f, 0.0f, 0.0f));

            GetWorld().SetComponent(camera, camTrans);
            GetWorld().SetComponent(camera, Camera(60.0f));
        }

        // --- Floor Entity ---
        {
            Entity floor = GetWorld().CreateEntity<Transform, LocalToWorld, MeshFilter, MeshRenderer>();

            GetWorld().SetComponent(floor, Transform(Vector3(0, 0, 0)));
            GetWorld().SetComponent(floor, MeshFilter(meshes[0]));
            GetWorld().SetComponent(floor, MeshRenderer(materials[0]));
        }

        // --- Cube Entity ---
        {
            cubeEntity = GetWorld().CreateEntity<Transform, LocalToWorld, MeshFilter, MeshRenderer>();

            GetWorld().SetComponent(cubeEntity, Transform(Vector3(0, 1.0f, 0)));
            GetWorld().SetComponent(cubeEntity, MeshFilter(meshes[1]));
            GetWorld().SetComponent(cubeEntity, MeshRenderer(materials[1]));
        }
    }

    void OnUpdate() override
    {
        // キューブを回転させるアニメーション
        if (GetWorld().IsAlive(cubeEntity))
        {
            Transform& t = GetWorld().GetComponent<Transform>(cubeEntity);

            // 修正: RotationAxisが存在しない可能性があるため、安全策としてAngleAxisまたは行列経由を試す
            // ここではよくある命名規則である "AngleAxis" を試してみます
            // もしこれでもエラーが出る場合は、SpanMath.hの中身を見せていただく必要があります
            Quaternion rot = Quaternion::AngleAxis(Vector3::Up, 0.01f);

            // 合成
            t.Rotation = t.Rotation * rot;
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