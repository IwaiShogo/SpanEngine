#include "Runtime/EntryPoint.h"
#include "Runtime/Application.h"
#include "Core/Log/Logger.h"
#include "Core/Math/SpanMath.h"

// --- Systems ---
#include "Runtime/Systems/Core/TransformSystem.h"
#include "Runtime/Systems/Graphics/CameraSystem.h"
#include "Runtime/Systems/Graphics/RenderingSystem.h"
#include "Runtime/Systems/Graphics/EditorCameraSystem.h"
#include "Runtime/Systems/Core/RelationshipSystem.h" // ★追加

// --- Components ---
#include "Runtime/Components/Core/Transform.h"
#include "Runtime/Components/Core/LocalToWorld.h"
#include "Runtime/Components/Core/Relationship.h" // ★Parent -> Relationship
#include "Runtime/Components/Graphics/MeshFilter.h"
#include "Runtime/Components/Graphics/MeshRenderer.h"
#include "Runtime/Components/Graphics/Camera.h"
#include "Runtime/Components/Editor/EditorCamera.h"

// --- Resources ---
#include "Runtime/Graphics/Resources/Mesh.h"
#include "Runtime/Graphics/Resources/Material.h"
#include "Runtime/Graphics/Resources/Texture.h"
#include "Runtime/Graphics/ModelLoader.h"

// --- Editor & Tools ---
#include "Editor/GuiManager.h"
#include "Editor/PanelManager.h"
#include "Runtime/ECS/Kernel/EntityBuilder.h" // ★追加: ビルダーを使う
#include "Editor/SelectionManager.h"

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

		// システム登録 (TransformSystemはRelationshipSystemの後に動くようにするか、依存関係に注意)
		// ※TransformSystemは削除してRelationshipSystemに統合した場合は登録不要
		GetWorld().AddSystem<EditorCameraSystem>();

		// ★RelationshipSystemとTransformSystemの登録
		// 今回の修正でRelationshipSystemは構造変更のみ、TransformSystemが行列計算を行うようになったので両方必要です
		GetWorld().AddSystem<RelationshipSystem>();
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

		// --- 床の作成 (Builder使用) ---
		{
			builtInMeshes.push_back(Mesh::CreatePlane(device, 20.0f, 20.0f));

			// ★修正: EntityBuilderで作成
			EntityBuilder(&GetWorld(), "Floor")
				.Add(MeshFilter(builtInMeshes[0]))
				.Add(MeshRenderer(materials[0]))
				.Add(LocalToWorld{}) // 描画に必要
				.Build();
		}

		// --- FBXモデルのロード ---
		loadedMeshes = ModelLoader::Load(device, "Assets/Y Bot.fbx");

		if (!loadedMeshes.empty())
		{
			// モデルのルートEntity (Builder使用)
			modelRoot = EntityBuilder(&GetWorld(), "Y Bot Model")
				.Add(LocalToWorld{})
				.With<Transform>([](Transform& t) {
				t.Scale = Vector3(0.01f, 0.01f, 0.01f); // 小さくする
					})
				.Build();

			// メッシュパーツの作成
			for (size_t i = 0; i < loadedMeshes.size(); i++)
			{
				std::string partName = "Part_" + std::to_string(i);

				Entity part = EntityBuilder(&GetWorld(), partName)
					.Add(MeshFilter(loadedMeshes[i]))
					.Add(MeshRenderer(materials[1]))
					.Add(LocalToWorld{})
					.Build();

				Entity a = part;
				Entity ab = part;
				
				// ★修正: RelationshipSystemを使って親子付け
				RelationshipSystem::SetParent(&GetWorld(), part, modelRoot);
			}

			// 初期選択
			SelectionManager::Select(modelRoot);
		}
		else
		{
			SPAN_ERROR("Failed to load model! Please check 'Assets/Y Bot.fbx' exists.");
		}

		// --- Camera (Builder使用) ---
		{
			EntityBuilder(&GetWorld(), "Main Camera")
				.Add(Camera(60.0f))
				.Add(EditorCamera{}) // 操作用
				.Add(LocalToWorld{}) // カメラシステムで必要
				.With<Transform>([](Transform& t) {
				t.Position = Vector3(0.0f, 2.0f, -5.0f);
				t.LookAt(Vector3(0.0f, 1.0f, 0.0f));
					})
				.With<Tag>([](Tag& t) {
				t.Value = "MainCamera"; // タグ設定
					})
				.Build();
		}

		// --- パネルはGuiManagerで自動生成されるため登録コードは不要 ---
	}

	void OnUpdate() override
	{
		// モデル回転 (デモ用)
		// RelationshipSystemになったのでTransformを直接いじればOK
		if (GetWorld().IsAlive(modelRoot))
		{
			// 回転させたい場合はコメントアウトを外す
			// Transform& t = GetWorld().GetComponent<Transform>(modelRoot);
			// t.Rotation = t.Rotation * Quaternion::AngleAxis(Vector3::Up, Time::GetDeltaTime() * 0.5f);
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
