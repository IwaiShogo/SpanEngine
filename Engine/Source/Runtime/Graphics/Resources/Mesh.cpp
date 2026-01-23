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

		// 位置(3), 法線(3), 色(3)
		std::vector<Vertex> vertices = {
			// 前面 (Z = -0.5) 法線: (0, 0, -1)
			{ {-w,	w, -w}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },
			{ { w,	w, -w}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },
			{ {-w, -w, -w}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },
			{ {-w, -w, -w}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },
			{ { w,	w, -w}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },
			{ { w, -w, -w}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },

			// 背面 (Z = +0.5) 法線: (0, 0, 1)
			{ {-w, -w,	w}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
			{ { w, -w,	w}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
			{ {-w,	w,	w}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
			{ {-w,	w,	w}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
			{ { w, -w,	w}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
			{ { w,	w,	w}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },

			// 上面 (Y = +0.5) 法線: (0, 1, 0)
			{ {-w,	w,	w}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
			{ { w,	w,	w}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
			{ {-w,	w, -w}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
			{ {-w,	w, -w}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
			{ { w,	w,	w}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
			{ { w,	w, -w}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },

			// 底面 (Y = -0.5) 法線: (0, -1, 0)
			{ {-w, -w, -w}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} },
			{ { w, -w, -w}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} },
			{ {-w, -w,	w}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} },
			{ {-w, -w,	w}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} },
			{ { w, -w, -w}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} },
			{ { w, -w,	w}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} },

			// 右面 (X = +0.5) 法線: (1, 0, 0)
			{ { w,	w, -w}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f} },
			{ { w,	w,	w}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f} },
			{ { w, -w, -w}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f} },
			{ { w, -w, -w}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f} },
			{ { w,	w,	w}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f} },
			{ { w, -w,	w}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f} },

			// 左面 (X = -0.5) 法線: (-1, 0, 0)
			{ {-w,	w,	w}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f} },
			{ {-w,	w, -w}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f} },
			{ {-w, -w,	w}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f} },
			{ {-w, -w,	w}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f} },
			{ {-w,	w, -w}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f} },
			{ {-w, -w, -w}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f} },
		};

		Mesh* mesh = new Mesh();
		mesh->Initialize(device, vertices);
		return mesh;
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
				auto GetVertex = [&](float p, float t) -> Vertex
					{
						float y = radius * std::cos(p);
						float r = radius * std::sin(p); // その高さでの半径
						float x = r * std::cos(t);
						float z = r * std::sin(t);

						Vector3 pos(x, y, z);
						Vector3 normal = pos * (1.0f / radius); // 正規化 (原点からの方向が法線になる)
						Vector3 color(1.0f, 1.0f, 1.0f);		// とりあえず白

						return { pos, normal, color };
					};

				// 4つの頂点を作成
				Vertex v1 = GetVertex(phi1, theta1); // 左上
				Vertex v2 = GetVertex(phi1, theta2); // 右上
				Vertex v3 = GetVertex(phi2, theta1); // 左下
				Vertex v4 = GetVertex(phi2, theta2); // 右下

				// 三角形1 (左上 -> 右上 -> 左下)
				vertices.push_back(v1);
				vertices.push_back(v2);
				vertices.push_back(v3);

				// 三角形2 (左下 -> 右上 -> 右下)
				vertices.push_back(v3);
				vertices.push_back(v2);
				vertices.push_back(v4);
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
			{ {-w, 0,  d}, {0, 1, 0}, {1, 1, 1} }, // 左奥
			{ { w, 0,  d}, {0, 1, 0}, {1, 1, 1} }, // 右奥
			{ {-w, 0, -d}, {0, 1, 0}, {1, 1, 1} }, // 左手前

			{ {-w, 0, -d}, {0, 1, 0}, {1, 1, 1} }, // 左手前
			{ { w, 0,  d}, {0, 1, 0}, {1, 1, 1} }, // 右奥
			{ { w, 0, -d}, {0, 1, 0}, {1, 1, 1} }, // 右手前
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
			float theta1 = 2.0f * Span::PI * i / slices;
			float theta2 = 2.0f * Span::PI * (i + 1) / slices;

			// 頂点位置計算
			float x1 = radius * std::cos(theta1);
			float z1 = radius * std::sin(theta1);
			float x2 = radius * std::cos(theta2);
			float z2 = radius * std::sin(theta2);

			// 法線 (側面なのでXZ平面の外向き)
			Vector3 n1 = Vector3(x1, 0, z1) * (1.0f / radius);
			Vector3 n2 = Vector3(x2, 0, z2) * (1.0f / radius);

			// 四角形 (2ポリゴン)
			// 左上 -> 右上 -> 左下
			vertices.push_back({ {x1,  h2, z1}, n1, {1,1,1} });
			vertices.push_back({ {x2,  h2, z2}, n2, {1,1,1} });
			vertices.push_back({ {x1, -h2, z1}, n1, {1,1,1} });

			// 左下 -> 右上 -> 右下
			vertices.push_back({ {x1, -h2, z1}, n1, {1,1,1} });
			vertices.push_back({ {x2,  h2, z2}, n2, {1,1,1} });
			vertices.push_back({ {x2, -h2, z2}, n2, {1,1,1} });
		}

		// 上面 (Top Cap) - 法線(0,1,0)
		for (int i = 0; i < slices; ++i)
		{
			float theta1 = 2.0f * Span::PI * i / slices;
			float theta2 = 2.0f * Span::PI * (i + 1) / slices;

			float x1 = radius * std::cos(theta1);
			float z1 = radius * std::sin(theta1);
			float x2 = radius * std::cos(theta2);
			float z2 = radius * std::sin(theta2);

			// 中心 -> 1 -> 2 の三角形
			vertices.push_back({ {0, h2, 0}, {0,1,0}, {1,1,1} });
			vertices.push_back({ {x2, h2, z2}, {0,1,0}, {1,1,1} }); // 時計回りに注意
			vertices.push_back({ {x1, h2, z1}, {0,1,0}, {1,1,1} });
		}

		// 底面 (Bottom Cap) - 法線(0,-1,0)
		for (int i = 0; i < slices; ++i)
		{
			float theta1 = 2.0f * Span::PI * i / slices;
			float theta2 = 2.0f * Span::PI * (i + 1) / slices;

			float x1 = radius * std::cos(theta1);
			float z1 = radius * std::sin(theta1);
			float x2 = radius * std::cos(theta2);
			float z2 = radius * std::sin(theta2);

			// 中心 -> 2 -> 1 (裏から見て時計回り＝表から見て反時計回りだが、底面は下を向くので)
			vertices.push_back({ {0, -h2, 0}, {0,-1,0}, {1,1,1} });
			vertices.push_back({ {x1, -h2, z1}, {0,-1,0}, {1,1,1} });
			vertices.push_back({ {x2, -h2, z2}, {0,-1,0}, {1,1,1} });
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

		// 側面 (Side)
		// 頂点(Apex)と底面の円周を結ぶ三角形
		for (int i = 0; i < slices; ++i)
		{
			float theta1 = 2.0f * Span::PI * i / slices;
			float theta2 = 2.0f * Span::PI * (i + 1) / slices;

			float x1 = radius * std::cos(theta1);
			float z1 = radius * std::sin(theta1);
			float x2 = radius * std::cos(theta2);
			float z2 = radius * std::sin(theta2);

			// 法線の計算 (傾きを考慮)
			// 簡易的に、底面頂点の外向きベクトルを少し上に傾ける
			// 厳密には (x, radius/height, z) の正規化
			float slope = radius / height;
			Vector3 n1 = Vector3(x1, radius, z1); // 簡易法線
			n1 = n1 * (1.0f / sqrt(n1.x * n1.x + n1.y * n1.y + n1.z * n1.z)); // 正規化
			Vector3 n2 = Vector3(x2, radius, z2);
			n2 = n2 * (1.0f / sqrt(n2.x * n2.x + n2.y * n2.y + n2.z * n2.z));

			// 頂点法線は (0,1,0) ではなく、側面の平均をとるのが綺麗だが、
			// ここでは簡易的に「各面の法線」を使用

			// 頂点(Top) -> 下2 -> 下1
			// 頂点の法線はどうする？ -> 側面の法線の平均 or 真上。今回は面法線に合わせる

			vertices.push_back({ {0, h2, 0},   n1, {1,1,1} }); // 頂点 (法線は近似)
			vertices.push_back({ {x2, -h2, z2}, n2, {1,1,1} });
			vertices.push_back({ {x1, -h2, z1}, n1, {1,1,1} });
		}

		// 底面 (Bottom Cap)
		for (int i = 0; i < slices; ++i)
		{
			float theta1 = 2.0f * Span::PI * i / slices;
			float theta2 = 2.0f * Span::PI * (i + 1) / slices;

			float x1 = radius * std::cos(theta1);
			float z1 = radius * std::sin(theta1);
			float x2 = radius * std::cos(theta2);
			float z2 = radius * std::sin(theta2);

			vertices.push_back({ {0, -h2, 0}, {0,-1,0}, {1,1,1} });
			vertices.push_back({ {x1, -h2, z1}, {0,-1,0}, {1,1,1} });
			vertices.push_back({ {x2, -h2, z2}, {0,-1,0}, {1,1,1} });
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
			float theta1 = 2.0f * Span::PI * i / segments;
			float theta2 = 2.0f * Span::PI * (i + 1) / segments;

			// リングの中心位置
			Vector3 center1 = { radius * std::cos(theta1), 0, radius * std::sin(theta1) };
			Vector3 center2 = { radius * std::cos(theta2), 0, radius * std::sin(theta2) };

			for (int j = 0; j < tubeSegments; ++j)
			{
				float phi1 = 2.0f * Span::PI * j / tubeSegments;
				float phi2 = 2.0f * Span::PI * (j + 1) / tubeSegments;

				auto GetTorusVertex = [&](float theta, float phi, Vector3 center) -> Vertex
					{
						// チューブ断面の円
						float x = std::cos(theta); // リング方向
						float z = std::sin(theta);

						// 断面のローカル座標
						float localX = tubeRadius * std::cos(phi);
						float localY = tubeRadius * std::sin(phi);

						// ワールド座標へ
						// center方向に localX だけ移動し、Y軸方向に localY だけ移動
						Vector3 pos = center + (Vector3(x, 0, z) * localX);
						pos.y += localY;

						// 法線: 中心から外側へ
						Vector3 normal = pos - center;
						normal = normal * (1.0f / tubeRadius); // 正規化

						return { pos, normal, {1,1,1} };
					};

				Vertex v1 = GetTorusVertex(theta1, phi1, center1);
				Vertex v2 = GetTorusVertex(theta2, phi1, center2); // 次のリング
				Vertex v3 = GetTorusVertex(theta1, phi2, center1);
				Vertex v4 = GetTorusVertex(theta2, phi2, center2);

				// Tri 1: 左上 -> 左下 -> 右上
				vertices.push_back(v1); vertices.push_back(v3); vertices.push_back(v2);
				
				// Tri 2: 右上 -> 左下 -> 右下
				vertices.push_back(v2); vertices.push_back(v3); vertices.push_back(v4);
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
		float cylinderHeight = std::max(0.0f, height - 2.0f * radius);
		float halfHeight = cylinderHeight * 0.5f;

		// ヘルパー: 四角形を追加するラムダ式 (頂点順序は時計回り: 左上->右上->左下, 右上->右下->左下)
		// ※Torusでの反省を活かし、時計回り(CW)で統一します
		auto PushQuad = [&](Vertex v1, Vertex v2, Vertex v3, Vertex v4) {
			vertices.push_back(v1); vertices.push_back(v2); vertices.push_back(v3);
			vertices.push_back(v2); vertices.push_back(v4); vertices.push_back(v3);
			};

		// --- 1. 上半球 (Top Hemisphere) ---
		for (int i = 0; i < stacks; ++i)
		{
			// 緯度: 0 (頂点) ～ PI/2 (赤道)
			float phi1 = (Span::PI * 0.5f) * i / stacks;
			float phi2 = (Span::PI * 0.5f) * (i + 1) / stacks;

			for (int j = 0; j < slices; ++j)
			{
				float theta1 = 2.0f * Span::PI * j / slices;
				float theta2 = 2.0f * Span::PI * (j + 1) / slices;

				auto GetTopVertex = [&](float p, float t) -> Vertex {
					float r = radius * std::sin(p);
					float y = radius * std::cos(p);
					float x = r * std::cos(t);
					float z = r * std::sin(t);

					// 法線: 球の中心からの方向
					Vector3 normal = Vector3(x, y, z) * (1.0f / radius);
					// 位置: 中心を halfHeight だけ上にずらす
					Vector3 pos = Vector3(x, y + halfHeight, z);

					return { pos, normal, {1,1,1} };
					};

				PushQuad(
					GetTopVertex(phi1, theta1), // 左上
					GetTopVertex(phi1, theta2), // 右上
					GetTopVertex(phi2, theta1), // 左下
					GetTopVertex(phi2, theta2)	// 右下
				);
			}
		}

		// --- 2. 円柱部分 (Cylinder Body) ---
		for (int j = 0; j < slices; ++j)
		{
			float theta1 = 2.0f * Span::PI * j / slices;
			float theta2 = 2.0f * Span::PI * (j + 1) / slices;

			float x1 = radius * std::cos(theta1);
			float z1 = radius * std::sin(theta1);
			float x2 = radius * std::cos(theta2);
			float z2 = radius * std::sin(theta2);

			// 法線 (XZ平面の外向き)
			Vector3 n1(x1 / radius, 0, z1 / radius);
			Vector3 n2(x2 / radius, 0, z2 / radius);

			// 上の縁 (y = +halfHeight)
			Vertex v1 = { {x1, halfHeight, z1}, n1, {1,1,1} };
			Vertex v2 = { {x2, halfHeight, z2}, n2, {1,1,1} };
			// 下の縁 (y = -halfHeight)
			Vertex v3 = { {x1, -halfHeight, z1}, n1, {1,1,1} };
			Vertex v4 = { {x2, -halfHeight, z2}, n2, {1,1,1} };

			PushQuad(v1, v2, v3, v4);
		}

		// --- 3. 下半球 (Bottom Hemisphere) ---
		for (int i = 0; i < stacks; ++i)
		{
			// 緯度: PI/2 (赤道) ～ PI (底)
			float phi1 = (Span::PI * 0.5f) + (Span::PI * 0.5f) * i / stacks;
			float phi2 = (Span::PI * 0.5f) + (Span::PI * 0.5f) * (i + 1) / stacks;

			for (int j = 0; j < slices; ++j)
			{
				float theta1 = 2.0f * Span::PI * j / slices;
				float theta2 = 2.0f * Span::PI * (j + 1) / slices;

				auto GetBottomVertex = [&](float p, float t) -> Vertex {
					float r = radius * std::sin(p);
					float y = radius * std::cos(p);
					float x = r * std::cos(t);
					float z = r * std::sin(t);

					Vector3 normal = Vector3(x, y, z) * (1.0f / radius);
					// 位置: 中心を halfHeight だけ下にずらす
					Vector3 pos = Vector3(x, y - halfHeight, z);

					return { pos, normal, {1,1,1} };
					};

				PushQuad(
					GetBottomVertex(phi1, theta1),
					GetBottomVertex(phi1, theta2),
					GetBottomVertex(phi2, theta1),
					GetBottomVertex(phi2, theta2)
				);
			}
		}

		Mesh* mesh = new Mesh();
		mesh->Initialize(device, vertices);
		return mesh;
	}
}