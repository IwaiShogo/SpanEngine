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
#include "Runtime/Components/Core/Relationship.h"
#include "Runtime/Components/Graphics/MeshFilter.h"
#include "Runtime/Components/Graphics/MeshRenderer.h"
#include "Runtime/Components/Graphics/Camera.h"
#include "Runtime/Components/Editor/EditorCamera.h"

// --- Resources ---
#include "Runtime/Graphics/Resources/Mesh.h"
#include "Runtime/Graphics/Resources/Material.h"
#include "Runtime/Graphics/Resources/Texture.h"
#include "Runtime/Graphics/ModelLoader.h" // ★追加

#include "Editor/SelectionManager.h"
#include "Runtime/Reflection/ComponentRegistry.h"
#include "Editor/ImGui/ImGuiUI.h"
#include "Editor/GuiManager.h"
#include "Editor/Panels/SceneViewPanel.h"
#include "Editor/Panels/InspectorPanel.h"

using namespace Span;

class PlaygroundApp : public Application
{
public:
    std::vector<Mesh*> loadedMeshes;
    std::vector<Mesh*> builtInMeshes;
    std::vector<Material*> materials;

    Entity modelRoot;

    void OnStart() override
    {
        SPAN_LOG("--- Playground App Started ---");

        // システム登録
        GetWorld().AddSystem<EditorCameraSystem>();
        GetWorld().AddSystem<TransformSystem>();
        GetWorld().AddSystem<CameraSystem>();
        GetWorld().AddSystem<RenderingSystem>();

        ID3D12Device* device = GetRenderer().GetDevice();

        // --- マテリアル作成 ---
        {
            Material* gray = new Material(); gray->Initialize(device); gray->SetAlbedo(Vector3(0.5f, 0.5f, 0.5f)); gray->SetRoughness(0.8f);
            materials.push_back(gray);

            Material* white = new Material(); white->Initialize(device); white->SetAlbedo(Vector3(1.0f, 1.0f, 1.0f));
            materials.push_back(white);
        }

        // --- 床の作成 ---
        {
            builtInMeshes.push_back(Mesh::CreatePlane(device, 20.0f, 20.0f));

            Entity floor = GetWorld().CreateEntity<Transform, LocalToWorld, MeshFilter, MeshRenderer>();
            GetWorld().SetComponent(floor, Transform(Vector3(0, 0, 0)));
            GetWorld().SetComponent(floor, MeshFilter(builtInMeshes[0]));
            GetWorld().SetComponent(floor, MeshRenderer(materials[0])); // グレー
        }

        // --- FBXモデルのロード ---
        // Assetsフォルダは実行ファイルと同じ場所にコピーされているはずです
        // ※パスは "Assets/Y Bot.fbx" です
        loadedMeshes = ModelLoader::Load(device, "Assets/Y Bot.fbx");

        if (loadedMeshes.empty())
        {
            SPAN_ERROR("Failed to load model! Please check 'Assets/Y Bot.fbx' exists.");
        }
        else
        {
            // モデルのルートEntityを作成 (操作用)
            modelRoot = GetWorld().CreateEntity<Transform, LocalToWorld>();
            GetWorld().SetComponent(modelRoot, Transform(Vector3(0, 0, 0))); // 原点

            // 読み込まれたメッシュごとにEntityを作成し、ルートの子にする
            for (Mesh* mesh : loadedMeshes)
            {
                Entity part = GetWorld().CreateEntity<Transform, LocalToWorld, MeshFilter, MeshRenderer, Parent>();

                // 親設定
                GetWorld().SetComponent(part, Parent(modelRoot));

                // メッシュ設定
                GetWorld().SetComponent(part, MeshFilter(mesh));
                GetWorld().SetComponent(part, MeshRenderer(materials[1])); // 白マテリアル

                // ローカル座標は初期値 (FBX内の階層構造はModelLoaderが平坦化してしまっているため、全て原点基準になります)
                // 本格対応はフェーズ4のアニメーション実装時に行います
                GetWorld().SetComponent(part, Transform::Identity());
            }

            // スケール調整 (Mixamoなどはデカいことが多いので 0.01倍 にしてみる)
            Transform& t = GetWorld().GetComponent<Transform>(modelRoot);
            t.Scale = Vector3(0.01f, 0.01f, 0.01f);
        }

        // --- Camera ---
        {
            Entity camera = GetWorld().CreateEntity<Transform, LocalToWorld, Camera, EditorCamera>();
            Transform t;
            t.Position = Vector3(0.0f, 2.0f, -5.0f); // モデルが見やすい位置へ
            t.LookAt(Vector3(0.0f, 1.0f, 0.0f));
            GetWorld().SetComponent(camera, t);
            GetWorld().SetComponent(camera, Camera(60.0f));
            GetWorld().SetComponent(camera, EditorCamera{});

            SelectionManager::Select(camera);
        }
    }

    void OnUpdate() override
    {
        // モデルをY軸回転させて鑑賞
        if (GetWorld().IsAlive(modelRoot))
        {
            Transform& t = GetWorld().GetComponent<Transform>(modelRoot);
            t.Rotation = t.Rotation * Quaternion::AngleAxis(Vector3::Up, Time::GetDeltaTime() * 0.5f);
        }
    }

    void OnShutdown() override
    {
        for (auto m : loadedMeshes) { m->Shutdown(); delete m; }
        for (auto m : builtInMeshes) { m->Shutdown(); delete m; }
        for (auto m : materials) { m->Shutdown(); delete m; }
        loadedMeshes.clear();
        builtInMeshes.clear();
        materials.clear();
    }
};

Application* Span::CreateApplication()
{
    return new PlaygroundApp();
}