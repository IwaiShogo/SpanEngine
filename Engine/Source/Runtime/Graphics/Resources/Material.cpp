/*****************************************************************//**
 * @file	Material.cpp
 * @brief	Materialの実装。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "Material.h"

namespace Span
{
	Material::Material()
	{
		// デフォルトパラメータの明示的初期化
		m_Data.AlbedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_Data.Roughness = 0.5f;
		m_Data.Metallic = 0.0f;
		m_Data.AO = 1.0f;
		m_Data.Cutoff = 0.5f;
		m_Data.Transmission = 0.0f;
		m_Data.Tiling = { 1.0f, 1.0f };
		m_Data.Offset = { 0.0f, 0.0f };
		m_Data.IOR = 1.5f;

		m_Data.HasAlbedoMap = 0;
		m_Data.HasNormalMap = 0;
		m_Data.HasMetallicMap = 0;
		m_Data.HasRoughnessMap = 0;
		m_Data.HasAOMap = 0;
		m_Data.HasEmissiveMap = 0;
	}

	Material::~Material()
	{
		Shutdown();
	}

	bool Material::Initialize(ID3D12Device* device)
	{
		m_ConstantBuffer = new ConstantBuffer<MaterialData>();
		return m_ConstantBuffer->Initialize(device);
	}

	void Material::Shutdown()
	{
		if (m_ConstantBuffer)
		{
			m_ConstantBuffer->Shutdown();
			SAFE_DELETE(m_ConstantBuffer);
		}
	}

	void Material::Update()
	{
		if (m_IsDirty && m_ConstantBuffer)
		{
			m_ConstantBuffer->Update(m_Data);
			m_IsDirty = false;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS Material::GetGPUVirtualAddress() const
	{
		return m_ConstantBuffer ? m_ConstantBuffer->GetGPUVirtualAddress() : 0;
	}

	bool Material::Serialize(const std::filesystem::path& filepath)
	{
		nlohmann::ordered_json j;

		j["Name"] = Name;
		j["BlendMode"] = static_cast<int>(m_BlendMode);
		j["CullMode"] = static_cast<int>(m_CullMode);

		j["Properties"] = {
			{"AlbedoColor", { m_Data.AlbedoColor.x, m_Data.AlbedoColor.y, m_Data.AlbedoColor.z, m_Data.AlbedoColor.w }},
			{"EmissiveColor", { m_Data.EmissiveColor.x, m_Data.EmissiveColor.y, m_Data.EmissiveColor.z }},
			{"Roughness", m_Data.Roughness},
			{"Metallic", m_Data.Metallic},
			{"AO", m_Data.AO},
			{"Cutoff", m_Data.Cutoff},
			{"Transmission", m_Data.Transmission},
			{"IOR", m_Data.IOR},
			{"Tiling", { m_Data.Tiling.x, m_Data.Tiling.y }},
			{"Offset", { m_Data.Offset.x, m_Data.Offset.y }}
		};

		// 実際のテクスチャポインタからGUIDを取得して保存する処理が将来必要ですが、
		// 現在はキャッシュされているGUID変数を保存します。
		j["Textures"] = {
			{"AlbedoMap", AlbedoMapGUID},
			{"NormalMap", NormalMapGUID},
			{"MetallicMap", MetallicMapGUID},
			{"RoughnessMap", RoughnessMapGUID},
			{"AOMap", AOMapGUID},
			{"EmissiveMap", EmissiveMapGUID}
		};

		std::ofstream fout(filepath);
		if (!fout.is_open()) return false;
		fout << std::setw(4) << j << std::endl;
		fout.close();

		return true;
	}

	bool Material::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream fin(filepath);
		if (!fin.is_open()) return false;

		nlohmann::ordered_json j;
		fin >> j;
		fin.close();

		Name = j.value("Name", "Unnamed Material");
		m_BlendMode = static_cast<BlendMode>(j.value("BlendMode", 0));
		m_CullMode = static_cast<CullMode>(j.value("CullMode", 0));

		if (j.contains("Properties"))
		{
			auto& p = j["Properties"];
			if (p.contains("AlbedoColor")) { m_Data.AlbedoColor = { p["AlbedoColor"][0], p["AlbedoColor"][1], p["AlbedoColor"][2], p["AlbedoColor"][3] }; }
			if (p.contains("EmissiveColor")) { m_Data.EmissiveColor = { p["EmissiveColor"][0], p["EmissiveColor"][1], p["EmissiveColor"][2] }; }
			m_Data.Roughness = p.value("Roughness", 0.5f);
			m_Data.Metallic = p.value("Metallic", 0.0f);
			m_Data.AO = p.value("AO", 1.0f);
			m_Data.Cutoff = p.value("Cutoff", 0.5f);
			m_Data.Transmission = p.value("Transmission", 0.0f);
			m_Data.IOR = p.value("IOR", 1.5f);
			if (p.contains("Tiling")) { m_Data.Tiling = { p["Tiling"][0], p["Tiling"][1] }; }
			if (p.contains("Offset")) { m_Data.Offset = { p["Offset"][0], p["Offset"][1] }; }
		}

		if (j.contains("Textures"))
		{
			auto& t = j["Textures"];
			AlbedoMapGUID = t.value("AlbedoMap", (uint64_t)0);
			NormalMapGUID = t.value("NormalMap", (uint64_t)0);
			MetallicMapGUID = t.value("MetallicMap", (uint64_t)0);
			RoughnessMapGUID = t.value("RoughnessMap", (uint64_t)0);
			AOMapGUID = t.value("AOMap", (uint64_t)0);
			EmissiveMapGUID = t.value("EmissiveMap", (uint64_t)0);

			// ※ロード完了後、AssetManager を通じて GUID から実際の Texture* を取得し、
			// SetAlbedoMap() 等を呼び出してリンクを張る「参照解決フェーズ」が別途必要です。
		}

		m_IsDirty = true;
		return true;
	}
}

