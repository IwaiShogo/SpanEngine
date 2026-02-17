/*****************************************************************//**
 * @file	AssetRegistry.cpp
 * @brief	AssetRegistryの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include <SpanEngine.h>
#include "AssetRegistry.h"
#include "AssetSerializer.h"

namespace Span
{
	AssetRegistry& AssetRegistry::Get()
	{
		static AssetRegistry instance;
		return instance;
	}

	void AssetRegistry::Refresh(const std::filesystem::path& rootDirectory)
	{
		m_Assets.clear();
		m_PathToHandle.clear();

		SPAN_LOG("AssetRegistry: Scanning assets in %s...", rootDirectory.string().c_str());

		if (!std::filesystem::exists(rootDirectory)) return;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDirectory))
		{
			if (entry.is_regular_file())
			{
				const auto& path = entry.path();

				// .meta ファイル自体は登録しない
				if (path.extension() == ".meta") continue;

				// 対応する .meta があるか確認して登録
				RegisterAsset(path);
			}
		}

		SPAN_LOG("AssetRegistry: Scan complete. %d assets indexed.", m_Assets.size());
	}

	void AssetRegistry::RegisterAsset(const std::filesystem::path& path)
	{
		// .meta が存在するか確認し、読み込む
		if (std::filesystem::exists(path.string() + ".meta"))
		{
			AssetMetadata meta = AssetSerializer::LoadOrCreateMetadata(path);
			if (meta.IsValid())
			{
				m_Assets[meta.Handle] = path;
				m_PathToHandle[path.string()] = meta.Handle;
			}
		}
	}

	void AssetRegistry::UnregisterAsset(const std::filesystem::path& path)
	{
		auto it = m_PathToHandle.find(path.string());
		if (it != m_PathToHandle.end())
		{
			m_Assets.erase(it->second);
			m_PathToHandle.erase(it);
		}
	}

	bool AssetRegistry::Contains(AssetHandle handle) const
	{
		return m_Assets.find(handle) != m_Assets.end();
	}

	bool AssetRegistry::Contains(const std::filesystem::path& path) const
	{
		return m_PathToHandle.find(path.string()) != m_PathToHandle.end();
	}

	const std::filesystem::path& AssetRegistry::GetPath(AssetHandle handle) const
	{
		auto it = m_Assets.find(handle);
		if (it != m_Assets.end()) return it->second;
		return m_EmptyPath;
	}

	AssetHandle AssetRegistry::GetHandle(const std::filesystem::path& path) const
	{
		auto it = m_PathToHandle.find(path.string());
		if (it != m_PathToHandle.end()) return it->second;
		return 0;
	}
}
