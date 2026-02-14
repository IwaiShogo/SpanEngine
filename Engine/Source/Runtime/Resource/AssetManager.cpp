/*****************************************************************//**
 * @file	AssetManager.cpp
 * @brief	AssetManagerの実装。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "AssetManager.h"

namespace Span
{
	AssetManager& AssetManager::Get()
	{
		static AssetManager instance;
		return instance;
	}

	void AssetManager::Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
	{
		m_Device = device;
		m_CommandQueue = cmdQueue;
		SPAN_LOG("AssetManager Initialized.");
	}

	void AssetManager::Shutdown()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		// 全テクスチャの明示的な解放
		for (auto& [key, texture] : m_TextureCache)
		{
			if (texture) texture->Shutdown();
		}
		m_TextureCache.clear();

		SPAN_LOG("AssetManager Shutdown: All assets released.");
	}

	std::shared_ptr<Texture> AssetManager::GetTexture(const std::string& path)
	{
		// デバイス未初期化チェック
		if (!m_Device || !m_CommandQueue)
		{
			SPAN_ERROR("AssetManager used before initialization!");
			return nullptr;
		}

		// パスを正規化してキーにする
		std::string key;
		try
		{
			key = std::filesystem::absolute(path).string();
		}
		catch (...)
		{
			key = path;
		}

		std::lock_guard<std::mutex> lock(m_Mutex);

		// 1. キャッシュヒット確認
		auto it = m_TextureCache.find(key);
		if (it != m_TextureCache.end())
		{
			return it->second;
		}

		// 2. 新規ロード
		if (!std::filesystem::exists(key))
		{
			SPAN_WARN("Asset not found: %s", path.c_str());
			return nullptr;
		}

		auto newTexture = std::make_shared<Texture>();
		if (newTexture->Initialize(m_Device, m_CommandQueue, key))
		{
			m_TextureCache[key] = newTexture;
			return newTexture;
		}

		SPAN_ERROR("Failed to load texture: %s", path.c_str());
		return nullptr;
	}

	std::shared_ptr<Mesh> AssetManager::GetMesh(const std::string& path)
	{
		std::string key;
		try { key = std::filesystem::absolute(path).string(); }
		catch (...) { key = path; }

		std::lock_guard<std::mutex> lock(m_Mutex);

		// 1. キャッシュ確認
		if (m_MeshCache.find(key) != m_MeshCache.end())
		{
			return m_MeshCache[key];
		}

		// 2. 新規ロード
		if (!std::filesystem::exists(key))
		{
			SPAN_ERROR("Mesh not found: %s", path.c_str());
			return nullptr;
		}

		std::vector<Mesh*> meshes = ModelLoader::Load(m_Device, key);

		if (!meshes.empty())
		{
			std::shared_ptr<Mesh> meshPtr(meshes[0]);

			m_MeshCache[key] = meshPtr;
			SPAN_LOG("Mesh Loaded: %s", path.c_str());

			for (size_t i = 1; i < meshes.size(); ++i)
			{
				delete meshes[i];
			}

			return meshPtr;
		}

		SPAN_ERROR("Failed to load mesh: %s", path.c_str());
		return nullptr;
	}

	std::shared_ptr<Material> AssetManager::GetDefaultMaterial()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_DefaultMaterial)
		{
			return m_DefaultMaterial;
		}

		if (!m_Device) return nullptr;

		if (!m_DefaultVS)
		{
			m_DefaultVS = std::make_shared<Shader>();
			if (!m_DefaultVS->Load(L"Basic.hlsl", ShaderType::Vertex, "VSMain"))
			{
				SPAN_ERROR("Failed to load Default Vertex Shader");
			}
		}

		if (!m_DefaultPS)
		{
			m_DefaultPS = std::make_shared<Shader>();
			if (!m_DefaultPS->Load(L"Basic.hlsl", ShaderType::Pixel, "PSMain"))
			{
				SPAN_ERROR("Failed to load Default Pixel Shader");
			}
		}

		// 2. マテリアルの生成
		m_DefaultMaterial = std::make_shared<Material>();

		if (m_DefaultMaterial->Initialize(m_Device))
		{
			m_DefaultMaterial->SetShaders(m_DefaultVS.get(), m_DefaultPS.get());

			// 色などを設定
			m_DefaultMaterial->SetAlbedo({ 1.0f, 1.0f, 1.0f });
			m_DefaultMaterial->Update();

			SPAN_LOG("Default Material Created.");
		}
		else
		{
			SPAN_ERROR("Failed to initialize Default Material");
			m_DefaultMaterial.reset();
		}

		return m_DefaultMaterial;
	}

	void* AssetManager::GetEditorThumbnail(const std::filesystem::path& path)
	{
		// 拡張子チェック
		std::string ext = path.extension().string();
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp")
		{
			// テクスチャとして取得
			auto tex = GetTexture(path.string());
			if (tex)
			{
				return tex->GetImGuiTextureID();
			}
		}

		// TODO: 将来的にはFBX, Prefabのプレビューなども分岐処理。
		return nullptr;
	}
}
