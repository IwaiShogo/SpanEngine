#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Math/SpanMath.h"

namespace Span
{
	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
		Vector3 color;
	};

	class Mesh
	{
	public:
		bool Initialize(ID3D12Device* device, const std::vector<Vertex>& vertices);
		void Shutdown();

		// 描画コマンドを発行
		void Draw(ID3D12GraphicsCommandList* commandList);

		// プリセット作成ヘルパー
		static Mesh* CreateCube(ID3D12Device* device);
		static Mesh* CreateSphere(ID3D12Device* device, int slices = 16, int stacks = 16);
		static Mesh* CreatePlane(ID3D12Device* device, float width = 10.0f, float depth = 10.0f);
		static Mesh* CreateCylinder(ID3D12Device* device, float radius = 0.5f, float height = 1.0f, int slices = 32);
		static Mesh* CreateCone(ID3D12Device* device, float radius = 0.5f, float height = 1.0f, int slices = 32);
		static Mesh* CreateTorus(ID3D12Device* device, float radius = 0.5f, float tubeRadius = 0.2f, int segments = 32, int tubeSegments = 16);
		static Mesh* CreateCapsule(ID3D12Device* device, float radius = 0.5f, float height = 2.0f, int slices = 32, int stacks = 16);

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		uint32 vertexCount = 0;
	};
}