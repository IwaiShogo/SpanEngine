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
#include "AssetRegistry.h"
#include "AssetSerializer.h"

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

		// レジストリの初期構築
		AssetRegistry::Get().Refresh("../Projects/Playground/Assets");

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
		m_MeshCache.clear();

		m_DefaultMaterial.reset();
		m_DefaultVS.reset();
		m_DefaultPS.reset();

		AssetRegistry::Get().Shutdown();

		SPAN_LOG("AssetManager Shutdown: All assets released.");
	}

	std::shared_ptr<Texture> AssetManager::GetTexture(AssetHandle handle)
	{
		if (handle == 0) return nullptr;

		std::lock_guard<std::mutex> lock(m_Mutex);

		// 1. キャッシュ確認
		auto it = m_TextureCache.find(handle);
		if (it != m_TextureCache.end()) return it->second;

		// 2. パス解決
		const auto& path = AssetRegistry::Get().GetPath(handle);
		if (path.empty())
		{
			SPAN_WARN("AssetManager: GUID %llu not found in Registry.", handle);
			return nullptr;
		}

		// 3. ロード
		// 将来的にはここで AsyncLoad
		auto texture = std::make_shared<Texture>();
		if (texture->Initialize(m_Device, m_CommandQueue, path.string()))
		{
			m_TextureCache[handle] = texture;
			return texture;
		}

		return nullptr;
	}

	std::shared_ptr<Mesh> AssetManager::GetMesh(AssetHandle handle)
	{
		if (handle == 0) return nullptr;
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_MeshCache.find(handle) != m_MeshCache.end()) return m_MeshCache[handle];

		const auto& path = AssetRegistry::Get().GetPath(handle);
		if (path.empty()) return nullptr;

		auto meshes = ModelLoader::Load(m_Device, path.string());
		if (!meshes.empty() && meshes[0] != nullptr)
		{
			m_MeshCache[handle] = std::shared_ptr<Mesh>(meshes[0]);
			m_MeshCache[handle]->SetPath(path.string());

			// 使用しなかった残りのメッシュを削除
			for (size_t i = 1; i < meshes.size(); ++i)
			{
				delete meshes[i];
			}

			return m_MeshCache[handle];
		}
		return nullptr;
	}

	std::shared_ptr<Texture> AssetManager::GetTexture(const std::string& path)
	{
		AssetHandle handle = AssetRegistry::Get().GetHandle(path);
		if (handle == 0)
		{
			AssetMetadata meta = AssetSerializer::LoadOrCreateMetadata(path);
			if (meta.IsValid())
			{
				AssetRegistry::Get().RegisterAsset(path);
				handle = meta.Handle;
			}
			else
			{
				return nullptr;
			}
		}
		return GetTexture(handle);
	}

	std::shared_ptr<Mesh> AssetManager::GetMesh(const std::string& path)
	{
		AssetHandle handle = AssetRegistry::Get().GetHandle(path);
		if (handle == 0)
		{
			AssetMetadata meta = AssetSerializer::LoadOrCreateMetadata(path);
			if (meta.IsValid())
			{
				AssetRegistry::Get().RegisterAsset(path);
				handle = meta.Handle;
			}
			else
			{
				return nullptr;
			}
		}

		return GetMesh(handle);
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
