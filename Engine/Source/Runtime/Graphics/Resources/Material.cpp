#include "Material.h"

namespace Span
{
	Material::Material()
	{

	}

	Material::~Material()
	{
		Shutdown();
	}

	bool Material::Initialize(ID3D12Device* device)
	{
		constantBuffer = new ConstantBuffer<MaterialData>();
		return constantBuffer->Initialize(device);
	}

	void Material::Shutdown()
	{
		if (constantBuffer)
		{
			constantBuffer->Shutdown();
			SAFE_DELETE(constantBuffer);
		}
	}

	void Material::Update()
	{
		if (isDirty && constantBuffer)
		{
			constantBuffer->Update(data);
			isDirty = false;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS Material::GetGPUVirtualAddress() const
	{
		return constantBuffer ? constantBuffer->GetGPUVirtualAddress() : 0;
	}
}

