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
#include <SpanEngine.h>

namespace Span
{
	SceneSerializer::SceneSerializer(Scene& scene)
		: m_Scene(scene)
	{}

	bool SceneSerializer::Serialize(const std::filesystem::path & filepath)
	{
		nlohmann::ordered_json sceneJson;
		sceneJson["SceneName"] = m_Scene.Name;

		// --- メタデータの保存 ---
		nlohmann::ordered_json metadataJson;
		metadataJson["MainCameraGUID"] = m_Scene.MainCameraGUID;

		metadataJson["EditorCameraState"] = {
			{"Position", { m_Scene.EditorCamera.Position[0], m_Scene.EditorCamera.Position[1], m_Scene.EditorCamera.Position[2] }},
			{"Pitch", m_Scene.EditorCamera.Pitch},
			{"Yaw", m_Scene.EditorCamera.Yaw}
		};
		sceneJson["Metadata"] = metadataJson;

		// --- エンティティの保存 ---
		nlohmann::ordered_json entitiesJson = nlohmann::json::array();
		auto entities = m_Scene.ECSWorld.GetAllEntities();
		for (auto entity : entities)
		{
			nlohmann::ordered_json entityJson;
			entityJson["EntityID"] = entity.ID.Index;

			nlohmann::ordered_json componentsJson;

			// レジストリに登録された全てのコンポーネントを走査し、持っていればJSON化
			for (const auto& meta : ComponentRegistry::GetAll())
			{
				if (meta.HasFunc(entity, m_Scene.ECSWorld))
				{
					nlohmann::ordered_json compJson;
					meta.SerializeFunc(entity, m_Scene.ECSWorld, compJson);

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

		// --- メタデータの復元 ---
		if (sceneJson.contains("SceneName")) {
			m_Scene.Name = sceneJson["SceneName"];
		}

		if (sceneJson.contains("Metadata")) {
			const auto& metadataJson = sceneJson["Metadata"];
			m_Scene.MainCameraGUID = metadataJson.value("MainCameraGUID", (uint64_t)0);

			if (metadataJson.contains("EditorCameraState")) {
				const auto& camState = metadataJson["EditorCameraState"];
				m_Scene.EditorCamera.Position[0] = camState["Position"][0];
				m_Scene.EditorCamera.Position[1] = camState["Position"][1];
				m_Scene.EditorCamera.Position[2] = camState["Position"][2];
				m_Scene.EditorCamera.Pitch = camState.value("Pitch", 15.0f);
				m_Scene.EditorCamera.Yaw = camState.value("Yaw", 0.0f);
			}
		}

		if (!sceneJson.contains("Entities")) return false;

		// 既存のシーンを真っ新にする
		m_Scene.ECSWorld.Clear();

		// 【第1パス】: エンティティの生成と基本コンポーネントの読み込み
		// -------------------------------------------------------------
		std::unordered_map<uint64_t, Entity> guidToEntityMap;

		for (const auto& entityJson : sceneJson["Entities"])
		{
			Entity entity = m_Scene.ECSWorld.CreateEntity<>();

			// JSONからGUIDを取得し、マップに登録 (一時IDではなく恒久IDを保存しておく)
			uint64_t uuid = entityJson["EntityID"].get<uint64_t>();
			guidToEntityMap[uuid] = entity;

			// IDComponentを手動で追加
			m_Scene.ECSWorld.AddComponent<IDComponent>(entity, IDComponent(uuid));

			if (entityJson.contains("Components"))
			{
				const auto& componentsJson = entityJson["Components"];
				for (const auto& meta : ComponentRegistry::GetAll())
				{
					// ここではRelationship以外のコンポーネントを復元
					if (meta.Name != "Relationship" && componentsJson.contains(meta.Name))
					{
						meta.AddFunc(entity, m_Scene.ECSWorld);
						meta.DeserializeFunc(entity, m_Scene.ECSWorld, componentsJson[meta.Name]);
					}
				}
			}
		}

		// 【第2パス】: 参照関係 (Relationship) の解決
		// -------------------------------------------------------------
		for (const auto& entityJson : sceneJson["Entities"])
		{
			if (entityJson.contains("Components") && entityJson["Components"].contains("Relationship"))
			{
				uint64_t uuid = entityJson["EntityID"].get<uint64_t>();
				Entity entity = guidToEntityMap[uuid];

				// Relationshipコンポーネントを追加
				m_Scene.ECSWorld.AddComponent<Relationship>(entity);
				auto& rel = m_Scene.ECSWorld.GetComponent<Relationship>(entity);

				const auto& relJson = entityJson["Components"]["Relationship"];

				// 保存されているGUIDを元に、第1パスで作ったマップから実際のEntityハンドルを復元
				auto ResolveEntity = [&](const std::string& key) -> Entity {
					uint64_t targetUUID = relJson.value(key, (uint64_t)0);
					return (targetUUID == 0 || guidToEntityMap.find(targetUUID) == guidToEntityMap.end())
						? Entity::Null : guidToEntityMap[targetUUID];
					};

				rel.Parent = ResolveEntity("Parent");
				rel.FirstChild = ResolveEntity("FirstChild");
				rel.PrevSibling = ResolveEntity("PrevSibling");
				rel.NextSibling = ResolveEntity("NextSibling");
			}
		}

		return true;
	}
}
