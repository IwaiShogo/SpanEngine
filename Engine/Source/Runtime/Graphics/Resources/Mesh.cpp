#include "Mesh.h"

namespace Span
{
	bool Mesh::Initialize(ID3D12Device* device, const std::vector<Vertex>& vertices)
	{
		vertexCount = static_cast<uint32>(vertices.size());
		uint32 sizeInBytes = vertexCount * sizeof(Vertex);

		// 1. アップロードヒープのプロパティ
		// CPUから書き込めて、GPUが読める場所
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		// 2. リソースの設定 (バッファ)
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// 3. バッファ作成
		if (FAILED(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer))))
		{
			SPAN_ERROR("Failed to create vertex buffer!");
			return false;
		}

		// 4. データをコピー (Map -> memcpy -> Unmap)
		void* pData;
		D3D12_RANGE readRange = { 0, 0 }; // CPUは読まない
		if (SUCCEEDED(vertexBuffer->Map(0, &readRange, &pData)))
		{
			memcpy(pData, vertices.data(), sizeInBytes);
			vertexBuffer->Unmap(0, nullptr);
		}

		// 5. ビューの作成
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(Vertex);
		vertexBufferView.SizeInBytes = sizeInBytes;

		return true;
	}

	void Mesh::Shutdown()
	{
		vertexBuffer.Reset();
	}

	void Mesh::Draw(ID3D12GraphicsCommandList* commandList)
	{
		// 頂点バッファをセットして描画
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->DrawInstanced(vertexCount, 1, 0, 0);
	}

	// --- プリセット実装 ---
	Mesh* Mesh::CreateCube(ID3D12Device* device)
	{
		const float w = 0.5f;
		// Vertex: { Pos, Normal, UV }
		std::vector<Vertex> vertices = {
			// Front (Z-)
			{ {-w, w, -w}, {0,0,-1}, {0,0} }, { {w, w, -w}, {0,0,-1}, {1,0} }, { {-w, -w, -w}, {0,0,-1}, {0,1} },
			{ {-w, -w, -w}, {0,0,-1}, {0,1} }, { {w, w, -w}, {0,0,-1}, {1,0} }, { {w, -w, -w}, {0,0,-1}, {1,1} },
			// Back (Z+)
			{ {-w, -w, w}, {0,0,1}, {1,1} }, { {w, -w, w}, {0,0,1}, {0,1} }, { {-w, w, w}, {0,0,1}, {1,0} },
			{ {-w, w, w}, {0,0,1}, {1,0} }, { {w, -w, w}, {0,0,1}, {0,1} }, { {w, w, w}, {0,0,1}, {0,0} },
			// Top (Y+)
			{ {-w, w, w}, {0,1,0}, {0,0} }, { {w, w, w}, {0,1,0}, {1,0} }, { {-w, w, -w}, {0,1,0}, {0,1} },
			{ {-w, w, -w}, {0,1,0}, {0,1} }, { {w, w, w}, {0,1,0}, {1,0} }, { {w, w, -w}, {0,1,0}, {1,1} },
			// Bottom (Y-)
			{ {-w, -w, -w}, {0,-1,0}, {0,0} }, { {w, -w, -w}, {0,-1,0}, {1,0} }, { {-w, -w, w}, {0,-1,0}, {0,1} },
			{ {-w, -w, w}, {0,-1,0}, {0,1} }, { {w, -w, -w}, {0,-1,0}, {1,0} }, { {w, -w, w}, {0,-1,0}, {1,1} },
			// Right (X+)
			{ {w, w, -w}, {1,0,0}, {0,0} }, { {w, w, w}, {1,0,0}, {1,0} }, { {w, -w, -w}, {1,0,0}, {0,1} },
			{ {w, -w, -w}, {1,0,0}, {0,1} }, { {w, w, w}, {1,0,0}, {1,0} }, { {w, -w, w}, {1,0,0}, {1,1} },
			// Left (X-)
			{ {-w, w, w}, {-1,0,0}, {0,0} }, { {-w, w, -w}, {-1,0,0}, {1,0} }, { {-w, -w, w}, {-1,0,0}, {0,1} },
			{ {-w, -w, w}, {-1,0,0}, {0,1} }, { {-w, w, -w}, {-1,0,0}, {1,0} }, { {-w, -w, -w}, {-1,0,0}, {1,1} },
		};
		Mesh* mesh = new Mesh(); mesh->Initialize(device, vertices); return mesh;
	}

	Mesh* Mesh::CreateSphere(ID3D12Device* device, int slices, int stacks)
	{
		std::vector<Vertex> vertices;
		float radius = 0.5f;

		// 緯度・経度でループ
		for (int i = 0; i < stacks; ++i)
		{
			// 緯度 (0 ～ PI)
			float phi1 = Span::PI * static_cast<float>(i) / stacks;
			float phi2 = Span::PI * static_cast<float>(i + 1) / stacks;

			for (int j = 0; j < slices; ++j)
			{
				// 経度 (0 ～ 2PI)
				float theta1 = 2.0f * Span::PI * static_cast<float>(j) / slices;
				float theta2 = 2.0f * Span::PI * static_cast<float>(j + 1) / slices;

				// 四角形の4点の座標と法線を計算
				// ヘルパーラムダ式: 球面上の点(p, t)の情報を計算してVertexを返す
				auto GetV = [&](float p, float t, float u, float v) -> Vertex
					{
						float r = radius * std::sin(p);
						Vector3 pos(r * std::cos(t), radius * std::cos(p), r * std::sin(t));
						return { pos, pos * (1.0f / radius), { u, v } };
					};

				float u1 = (float)j / slices, u2 = (float)(j + 1) / slices;
				float v1 = (float)i / stacks, v2 = (float)(i + 1) / stacks;
				Vertex v_tl = GetV(phi1, theta1, u1, v1), v_tr = GetV(phi1, theta2, u2, v1);
				Vertex v_bl = GetV(phi2, theta1, u1, v2), v_br = GetV(phi2, theta2, u2, v2);
				vertices.push_back(v_tl); vertices.push_back(v_tr); vertices.push_back(v_bl);
				vertices.push_back(v_bl); vertices.push_back(v_tr); vertices.push_back(v_br);
			}
		}

		Mesh* mesh = new Mesh();
		mesh->Initialize(device, vertices);
		return mesh;
	}

	// --- 平面 (Plane/Quad) ---
	Mesh* Mesh::CreatePlane(ID3D12Device* device, float width, float depth)
	{
		float w = width * 0.5f;
		float d = depth * 0.5f;

		// 上向き(0,1,0)の大きな四角形
		std::vector<Vertex> vertices = {
			{ {-w, 0, d}, {0,1,0}, {0,0} }, { {w, 0, d}, {0,1,0}, {1,0} }, { {-w, 0, -d}, {0,1,0}, {0,1} },
			{ {-w, 0, -d}, {0,1,0}, {0,1} }, { {w, 0, d}, {0,1,0}, {1,0} }, { {w, 0, -d}, {0,1,0}, {1,1} }
		};

		Mesh* mesh = new Mesh();
		mesh->Initialize(device, vertices);
		return mesh;
	}

	// --- 円柱 (Cylinder) ---
	Mesh* Mesh::CreateCylinder(ID3D12Device* device, float radius, float height, int slices)
	{
		std::vector<Vertex> vertices;
		float h2 = height * 0.5f;

		// 側面 (Side)
		for (int i = 0; i < slices; ++i)
		{
			float t1 = 2.0f * Span::PI * i / slices;
			float t2 = 2.0f * Span::PI * (i + 1) / slices;
			float u1 = (float)i / slices;
			float u2 = (float)(i + 1) / slices;

			// 頂点位置計算
			float x1 = radius * std::cos(t1);
			float z1 = radius * std::sin(t1);
			float x2 = radius * std::cos(t2);
			float z2 = radius * std::sin(t2);

			// 法線 (側面なのでXZ平面の外向き)
			Vector3 n1(x1 / radius, 0, z1 / radius);
			Vector3 n2(x2 / radius, 0, z2 / radius);

			// Side
			vertices.push_back({ {x1, h2, z1}, n1, {u1, 0} });
			vertices.push_back({ {x2, h2, z2}, n2, {u2, 0} });
			vertices.push_back({ {x1, -h2, z1}, n1, {u1, 1} });
			vertices.push_back({ {x1, -h2, z1}, n1, {u1, 1} });
			vertices.push_back({ {x2, h2, z2}, n2, {u2, 0} });
			vertices.push_back({ {x2, -h2, z2}, n2, {u2, 1} });

			// Caps
			vertices.push_back({ {0, h2, 0}, {0,1,0}, {0.5f, 0.5f} });
			vertices.push_back({ {x2, h2, z2}, {0,1,0}, {0.5f + x2 / radius / 2, 0.5f + z2 / radius / 2} });
			vertices.push_back({ {x1, h2, z1}, {0,1,0}, {0.5f + x1 / radius / 2, 0.5f + z1 / radius / 2} });
			vertices.push_back({ {0, -h2, 0}, {0,-1,0}, {0.5f, 0.5f} });
			vertices.push_back({ {x1, -h2, z1}, {0,-1,0}, {0.5f + x1 / radius / 2, 0.5f + z1 / radius / 2} });
			vertices.push_back({ {x2, -h2, z2}, {0,-1,0}, {0.5f + x2 / radius / 2, 0.5f + z2 / radius / 2} });
		}

		Mesh* mesh = new Mesh();
		mesh->Initialize(device, vertices);
		return mesh;
	}

	// --- 円錐 (Cone) ---
	Mesh* Mesh::CreateCone(ID3D12Device* device, float radius, float height, int slices)
	{
		std::vector<Vertex> vertices;
		float h2 = height * 0.5f;

		for (int i = 0; i < slices; ++i)
		{
			float t1 = 2.0f * Span::PI * i / slices;
			float t2 = 2.0f * Span::PI * (i + 1) / slices;

			float x1 = radius * std::cos(t1);
			float z1 = radius * std::sin(t1);
			float x2 = radius * std::cos(t2);
			float z2 = radius * std::sin(t2);

			// 法線の計算 (傾きを考慮)
			Vector3 n1 = Vector3(x1, radius, z1) * (1.0f / sqrt(x1 * x1 + radius * radius + z1 * z1));
			Vector3 n2 = Vector3(x2, radius, z2) * (1.0f / sqrt(x2 * x2 + radius * radius + z2 * z2));

			vertices.push_back({ {0, h2, 0}, n1, {0.5f, 0} });
			vertices.push_back({ {x2, -h2, z2}, n2, { (float)(i + 1) / slices, 1} });
			vertices.push_back({ {x1, -h2, z1}, n1, { (float)i / slices, 1} });
			vertices.push_back({ {0, -h2, 0}, {0,-1,0}, {0.5f, 0.5f} });
			vertices.push_back({ {x1, -h2, z1}, {0,-1,0}, {0.5f + x1 / radius / 2, 0.5f + z1 / radius / 2} });
			vertices.push_back({ {x2, -h2, z2}, {0,-1,0}, {0.5f + x2 / radius / 2, 0.5f + z2 / radius / 2} });
		}

		Mesh* mesh = new Mesh();
		mesh->Initialize(device, vertices);
		return mesh;
	}

	// --- ドーナツ型 (Torus) ---
	Mesh* Mesh::CreateTorus(ID3D12Device* device, float radius, float tubeRadius, int segments, int tubeSegments)
	{
		std::vector<Vertex> vertices;

		for (int i = 0; i < segments; ++i)
		{
			for (int j = 0; j < tubeSegments; ++j)
			{
				float u1 = (float)i / segments;
				float u2 = (float)(i + 1) / segments;
				float v1 = (float)j / tubeSegments;
				float v2 = (float)(j + 1) / tubeSegments;

				auto GetV = [&](float u, float v) -> Vertex
					{
						float t = u * 2 * PI;
						float p = v * 2 * PI;
						float cx = radius * cos(t);
						float cz = radius * sin(t);

						Vector3 c(cx, 0, cz);
						Vector3 pos = c + Vector3(cos(t), 0, sin(t)) * (tubeRadius * cos(p));

						pos.y += tubeRadius * sin(p);
						return { pos, (pos - c) * (1.0f / tubeRadius), {u, v} };
					};
				Vertex v1v = GetV(u1, v1);
				Vertex v2v = GetV(u2, v1);
				Vertex v3v = GetV(u1, v2);
				Vertex v4v = GetV(u2, v2);

				vertices.push_back(v1v);
				vertices.push_back(v3v);
				vertices.push_back(v2v);
				vertices.push_back(v2v);
				vertices.push_back(v3v);
				vertices.push_back(v4v);
			}
		}

		Mesh* mesh = new Mesh();
		mesh->Initialize(device, vertices);
		return mesh;
	}

	Mesh* Mesh::CreateCapsule(ID3D12Device* device, float radius, float height, int slices, int stacks)
	{
		std::vector<Vertex> vertices;

		// 円柱部分の高さ (全長 - 上下の半径)
		// ※ height が 2*radius より小さい場合は球体になります
		float cylinderHeight = std::max(0.0f, height - 2.0f * radius);
		float halfHeight = cylinderHeight * 0.5f;

		// UVマッピングのための全体長さを計算（球部分は円周の長さで近似）
		// これによりテクスチャが伸び縮みせず均一に貼れます
		float sphereArcLen = radius * Span::PI * 0.5f; // 半球の弧長
		float totalLen = cylinderHeight + 2.0f * sphereArcLen;

		// V座標の境界値
		float vTopEnd = sphereArcLen / totalLen;		   // 上半球の終わり
		float vBottomStart = (sphereArcLen + cylinderHeight) / totalLen; // 下半球の始まり

		// ヘルパー: 頂点情報を生成するラムダ式
		auto GetVertex = [&](float x, float y, float z, float u, float v, float nY_offset) -> Vertex
			{
				Vector3 pos(x, y, z);
				// 法線の計算: 円柱部分はXZ平面、球部分は中心からの方向
				// ここでは球の中心オフセットを引いて正規化することで汎用的に計算
				Vector3 center(0, nY_offset, 0);
				Vector3 normal = pos - center;
				// 正規化 (手動)
				float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
				if (len > 0.0f) normal = normal * (1.0f / len);

				return { pos, normal, {u, v} };
			};

		// --------------------------------------------------------
		// 1. 上半球 (Top Hemisphere)
		// --------------------------------------------------------
		for (int i = 0; i < stacks; ++i)
		{
			float phi1 = Span::PI * 0.5f * i / stacks;
			float phi2 = Span::PI * 0.5f * (i + 1) / stacks;

			for (int j = 0; j < slices; ++j)
			{
				float theta1 = 2.0f * Span::PI * j / slices;
				float theta2 = 2.0f * Span::PI * (j + 1) / slices;

				float u1 = (float)j / slices;
				float u2 = (float)(j + 1) / slices;

				// V座標は 0.0 ～ vTopEnd にマッピング
				float v1 = (float)i / stacks * vTopEnd;
				float v2 = (float)(i + 1) / stacks * vTopEnd;

				// 四角形の4点計算
				auto CalcSpherePoint = [&](float phi, float theta) -> Vector3 {
					float r = radius * std::sin(phi);
					return Vector3(r * std::cos(theta), radius * std::cos(phi) + halfHeight, r * std::sin(theta));
					};

				Vector3 p1 = CalcSpherePoint(phi1, theta1);
				Vector3 p2 = CalcSpherePoint(phi1, theta2);
				Vector3 p3 = CalcSpherePoint(phi2, theta1);
				Vector3 p4 = CalcSpherePoint(phi2, theta2);

				// 中心オフセットは halfHeight
				Vertex v_tl = GetVertex(p1.x, p1.y, p1.z, u1, v1, halfHeight);
				Vertex v_tr = GetVertex(p2.x, p2.y, p2.z, u2, v1, halfHeight);
				Vertex v_bl = GetVertex(p3.x, p3.y, p3.z, u1, v2, halfHeight);
				Vertex v_br = GetVertex(p4.x, p4.y, p4.z, u2, v2, halfHeight);

				vertices.push_back(v_tl); vertices.push_back(v_tr); vertices.push_back(v_bl);
				vertices.push_back(v_bl); vertices.push_back(v_tr); vertices.push_back(v_br);
			}
		}

		// --------------------------------------------------------
		// 2. 円柱部分 (Cylinder Body)
		// --------------------------------------------------------
		if (cylinderHeight > 0.0f)
		{
			for (int j = 0; j < slices; ++j)
			{
				float theta1 = 2.0f * Span::PI * j / slices;
				float theta2 = 2.0f * Span::PI * (j + 1) / slices;

				float u1 = (float)j / slices;
				float u2 = (float)(j + 1) / slices;

				// V座標は vTopEnd ～ vBottomStart
				float v1 = vTopEnd;
				float v2 = vBottomStart;

				float x1 = radius * std::cos(theta1);
				float z1 = radius * std::sin(theta1);
				float x2 = radius * std::cos(theta2);
				float z2 = radius * std::sin(theta2);

				// 上の縁 (y = +halfHeight), 法線中心Y = y
				// 円柱側面の法線は (x, 0, z) なので、Yオフセットをその点のYにすればよい
				Vertex v_tl = GetVertex(x1, halfHeight, z1, u1, v1, halfHeight);
				Vertex v_tr = GetVertex(x2, halfHeight, z2, u2, v1, halfHeight);
				Vertex v_bl = GetVertex(x1, -halfHeight, z1, u1, v2, -halfHeight);
				Vertex v_br = GetVertex(x2, -halfHeight, z2, u2, v2, -halfHeight);

				vertices.push_back(v_tl); vertices.push_back(v_tr); vertices.push_back(v_bl);
				vertices.push_back(v_bl); vertices.push_back(v_tr); vertices.push_back(v_br);
			}
		}

		// --------------------------------------------------------
		// 3. 下半球 (Bottom Hemisphere)
		// --------------------------------------------------------
		for (int i = 0; i < stacks; ++i)
		{
			// 緯度: PI/2 ～ PI
			float phi1 = (Span::PI * 0.5f) + (Span::PI * 0.5f) * i / stacks;
			float phi2 = (Span::PI * 0.5f) + (Span::PI * 0.5f) * (i + 1) / stacks;

			for (int j = 0; j < slices; ++j)
			{
				float theta1 = 2.0f * Span::PI * j / slices;
				float theta2 = 2.0f * Span::PI * (j + 1) / slices;

				float u1 = (float)j / slices;
				float u2 = (float)(j + 1) / slices;

				// V座標: vBottomStart ～ 1.0
				float v1 = vBottomStart + (float)i / stacks * (1.0f - vBottomStart);
				float v2 = vBottomStart + (float)(i + 1) / stacks * (1.0f - vBottomStart);

				auto CalcSpherePoint = [&](float phi, float theta) -> Vector3 {
					float r = radius * std::sin(phi);
					// 下半球なのでオフセットは -halfHeight
					return Vector3(r * std::cos(theta), radius * std::cos(phi) - halfHeight, r * std::sin(theta));
					};

				Vector3 p1 = CalcSpherePoint(phi1, theta1);
				Vector3 p2 = CalcSpherePoint(phi1, theta2);
				Vector3 p3 = CalcSpherePoint(phi2, theta1);
				Vector3 p4 = CalcSpherePoint(phi2, theta2);

				// 中心オフセットは -halfHeight
				Vertex v_tl = GetVertex(p1.x, p1.y, p1.z, u1, v1, -halfHeight);
				Vertex v_tr = GetVertex(p2.x, p2.y, p2.z, u2, v1, -halfHeight);
				Vertex v_bl = GetVertex(p3.x, p3.y, p3.z, u1, v2, -halfHeight);
				Vertex v_br = GetVertex(p4.x, p4.y, p4.z, u2, v2, -halfHeight);

				vertices.push_back(v_tl); vertices.push_back(v_tr); vertices.push_back(v_bl);
				vertices.push_back(v_bl); vertices.push_back(v_tr); vertices.push_back(v_br);
			}
		}

		Mesh* mesh = new Mesh();
		mesh->Initialize(device, vertices);
		return mesh;
	}
}