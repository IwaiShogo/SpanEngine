/*****************************************************************//**
 * @file	SceneSerializer.cpp
 * @brief	SceneSerializerの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "SceneSerializer.h"
#include "Runtime/Reflection/ComponentRegistry.h"

namespace Span
{
	SceneSerializer::SceneSerializer(World& world)
		: m_World(world)
	{}

	bool SceneSerializer::Serialize(const std::filesystem::path & filepath)
	{
		nlohmann::ordered_json sceneJson;
		sceneJson["Scene"] = filepath.stem().string();

		nlohmann::ordered_json entitiesJson = nlohmann::json::array();

		auto entities = m_World.GetAllEntities();
		for (auto entity : entities)
		{
			nlohmann::ordered_json entityJson;
			entityJson["EntityID"] = entity.ID.Index;

			nlohmann::ordered_json componentsJson;

			// レジストリに登録された全てのコンポーネントを走査し、持っていればJSON化
			for (const auto& meta : ComponentRegistry::GetAll())
			{
				if (meta.HasFunc(entity, m_World))
				{
					nlohmann::ordered_json compJson;
					meta.SerializeFunc(entity, m_World, compJson);

					// JSONがから出ない場合
					if (!compJson.is_null())
					{
						componentsJson[meta.Name] = compJson;
					}
				}
			}

			entityJson["Components"] = componentsJson;
			entitiesJson.push_back(entityJson);
		}

		sceneJson["Entities"] = entitiesJson;

		std::ofstream fout(filepath);
		if (!fout.is_open())
		{
			return false;
		}

		fout << std::setw(4) << sceneJson << std::endl;
		fout.close();

		return true;
	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream fin(filepath);
		if (!fin.is_open()) return false;

		nlohmann::ordered_json sceneJson;
		fin >> sceneJson;
		fin.close();

		if (!sceneJson.contains("Entities")) return false;

		// 既存のシーンを真っ新にする
		m_World.Clear();

		// JSONからエンティティを復元
		for (const auto& entityJson : sceneJson["Entities"])
		{
			// 空のエンティティを生成
			Entity entity = m_World.CreateEntity<>();

			if (entityJson.contains("Components"))
			{
				const auto& componentsJson = entityJson["Components"];

				for (const auto& meta : ComponentRegistry::GetAll())
				{
					if (componentsJson.contains(meta.Name))
					{
						// エンティティにコンポーネントを追加
						meta.AddFunc(entity, m_World);
						// JSONデータをコンポーネントの構造体に流し込む
						meta.DeserializeFunc(entity, m_World, componentsJson[meta.Name]);
					}
				}
			}
		}

		return true;
	}
}
