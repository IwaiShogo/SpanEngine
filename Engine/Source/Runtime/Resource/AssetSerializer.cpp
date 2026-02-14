/*****************************************************************//**
 * @file	AssetSerializer.cpp
 * @brief	AssetSerializerの実装。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include <SpanEngine.h>
#include "AssetSerializer.h"

namespace Span
{
	// ランダムのGUID生成器
	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;


	AssetMetadata Span::AssetSerializer::LoadOrCreateMetadata(const std::filesystem::path& assetPath)
	{
		std::filesystem::path metaPath = assetPath;
		metaPath += ".meta";

		AssetMetadata metadata;

		// 1. 既存の.metaを読み込む
		if (std::filesystem::exists(metaPath))
		{
			std::ifstream stream(metaPath);
			if (stream.is_open())
			{
				std::string line;
				while (std::getline(stream, line))
				{
					if (line.find("GUID:") != std::string::npos)
					{
						try
						{
							metadata.Handle = std::stoull(line.substr(6));
						}
						catch (...) { metadata.Handle = 0; }
					}
					else if (line.find("Type:") != std::string::npos)
					{
						try
						{
							metadata.Type = (AssetType)std::stoi(line.substr(6));
						}
						catch (...) { metadata.Type = AssetType::None; }
					}
				}
				stream.close();
			}
		}

		// 2. 読み込み失敗、または存在しない場合は新規作成
		if (!metadata.IsValid())
		{
			metadata.Handle = s_UniformDistribution(s_Engine);
			if (metadata.Handle == 0) metadata.Handle++;

			metadata.Type = DeduceTypeFromExtension(assetPath);

			SaveMetadata(assetPath, metadata);
			SPAN_LOG("Generated .meta for: %s", assetPath.string().c_str());
		}

		return metadata;
	}

	void AssetSerializer::SaveMetadata(const std::filesystem::path& assetPath, const AssetMetadata& metadata)
	{
		std::filesystem::path metaPath = assetPath;
		metaPath += ".meta";

		std::ofstream stream(metaPath);
		if (stream.is_open())
		{
			stream << "GUID: " << metadata.Handle << "\n";
			stream << "Type: " << (int)metadata.Type << "\n";
			stream.close();
		}
	}

	AssetType AssetSerializer::DeduceTypeFromExtension(const std::filesystem::path& path)
	{
		std::string ext = path.extension().string();
		if (ext == ".png" || ext == ".jpg" || ext == ".tga")	return AssetType::Texture;
		if (ext == ".fbx" || ext == ".obj")						return AssetType::Mesh;
		if (ext == ".mat")										return AssetType::Material;
		if (ext == ".span")										return AssetType::Scene;
		if (ext == ".h" || ext == ".cpp")						return AssetType::Script;
		return AssetType::None;
	}
}
