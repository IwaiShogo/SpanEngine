#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <SpanEngine.h>
#include <SpanEditor.h>
#include "Runtime/EntryPoint.h"

using namespace Span;

class PlaygroundApp : public Application
{
public:
	// リソース管理用リスト (終了時に一括解放するため)
	std::vector<Mesh*>     m_meshes;
	std::vector<Material*> m_materials;
	std::vector<Texture*>  m_textures;

	Entity modelRoot;

	void OnStart() override
	{
#if defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

		SPAN_LOG("--- Playground App Started ---");

		// システム登録
		GetWorld().AddSystem<EditorCameraSystem>();
		GetWorld().AddSystem<RelationshipSystem>();
		GetWorld().AddSystem<TransformSystem>();
		GetWorld().AddSystem<CameraSystem>();
		GetWorld().AddSystem<RenderingSystem>();

		// 【修正1】Renderer経由でデバイスとコマンドキューを取得
		ID3D12Device* device = GetRenderer().GetDevice();
		ID3D12CommandQueue* commandQueue = GetRenderer().GetCommandQueue();

		// --- テクスチャ作成 ---
		Texture* testTexture = new Texture();
		// 【修正1】取得した device, commandQueue を使用
		if (testTexture->Initialize(device, commandQueue, "Assets/test.jpg"))
		{
			m_textures.push_back(testTexture);
		}
		else
		{
			delete testTexture;
			testTexture = nullptr;
		}

		// --- マテリアル作成 ---
		{
			Material* gray = new Material();
			gray->Initialize(device);
			gray->GetData().AlbedoColor = Vector4(1.0f, 0.5f, 0.3f, 1.0f);
			gray->GetData().Roughness = 0.5f;
			if (testTexture) gray->SetAlbedoMap(testTexture); // テクスチャ適用
			m_materials.push_back(gray);

			Material* white = new Material();
			white->Initialize(device);
			white->GetData().AlbedoColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
			m_materials.push_back(white);
		}

		// --- 床の作成 (Builder使用) ---
		{
			Mesh* planeMesh = Mesh::CreatePlane(device, 20.0f, 20.0f);
			m_meshes.push_back(planeMesh);

			EntityBuilder(&GetWorld(), "Floor")
				.Add(MeshFilter(planeMesh))
				.Add(MeshRenderer(m_materials[0]))
				.Add(LocalToWorld{})
				.Build();
		}

		// --- FBXモデルのロード ---
		// Load関数で生成されたメッシュを受け取り、管理リストに追加
		std::vector<Mesh*> loaded = ModelLoader::Load(device, "Assets/Y Bot.fbx");
		m_meshes.insert(m_meshes.end(), loaded.begin(), loaded.end());

		if (!loaded.empty())
		{
			// モデルのルートEntity
			modelRoot = EntityBuilder(&GetWorld(), "Y Bot Model")
				.Add(LocalToWorld{})
				.With<Transform>([](Transform& t) {
				t.Scale = Vector3(0.01f, 0.01f, 0.01f);
					})
				.Build();

			// メッシュパーツの作成
			for (size_t i = 0; i < loaded.size(); i++)
			{
				std::string partName = "Part_" + std::to_string(i);

				Entity part = EntityBuilder(&GetWorld(), partName)
					// MeshFilterにメッシュ、MeshRendererにマテリアルを個別に渡す
					.Add(MeshFilter(loaded[i]))
					.Add(MeshRenderer(m_materials[1]))
					.Add(LocalToWorld{})
					.Build();

				// 親子付け
				RelationshipSystem::SetParent(&GetWorld(), part, modelRoot);
			}

			// 初期選択
			SelectionManager::Select(modelRoot);
		}
		else
		{
			SPAN_ERROR("Failed to load model! Please check 'Assets/Y Bot.fbx' exists.");
		}

		// --- Camera ---
		{
			EntityBuilder(&GetWorld(), "Main Camera")
				.Add(Camera(60.0f))
				.Add(EditorCamera{})
				.Add(LocalToWorld{})
				.With<Transform>([](Transform& t) {
				t.Position = Vector3(0.0f, 2.0f, -5.0f);
				t.LookAt(Vector3(0.0f, 1.0f, 0.0f));
					})
				.With<Tag>([](Tag& t) {
				t.Value = "MainCamera";
					})
				.Build();
		}
	}

	void OnUpdate() override
	{
		// デモ用回転ロジック
		// if (GetWorld().IsAlive(modelRoot)) { ... }
	}

	void OnShutdown() override
	{
		// 【重要】メモリリーク防止：リソースの解放処理
		// 生成順と逆順に解放するのが安全です

		// 1. マテリアル
		for (auto m : m_materials) {
			if (m) {
				m->Shutdown();
				delete m;
			}
		}
		m_materials.clear();

		// 2. テクスチャ
		for (auto t : m_textures) {
			if (t) {
				t->Shutdown();
				delete t;
			}
		}
		m_textures.clear();

		// 3. メッシュ (Live Object警告の原因となるため確実に削除)
		for (auto m : m_meshes) {
			if (m) {
				m->Shutdown();
				delete m;
			}
		}
		m_meshes.clear();
	}
};

Application* Span::CreateApplication()
{
	return new PlaygroundApp();
}
