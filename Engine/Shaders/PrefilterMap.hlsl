// =========================================================================
// Span Engine - Prefilter Environment Map Generator
// =========================================================================

static const float PI = 3.14159265359f;

// 定数バッファ
cbuffer RootConstants : register(b0)
{
	float Roughness;
	float Resolution;
}

// 入力
TextureCube environmentMap : register(t0);
SamplerState defaultSampler : register(s0);

// 出力
RWTexture2DArray<float4> prefilterMap : register(u0);

// =========================================================================
// PBR Math Utilities
// =========================================================================

// Van der Corput
float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Hammersleyセット
float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

// GGX法線分布に基づく重点サンプリングベクトルを計算
float ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0f * PI * Xi.x;
	float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	
	// 球面座標からデカルト座標へ
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// 接空間からワールド空間へ変換
	float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

// =========================================================================
// Main Compute Shader
// =========================================================================

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint width, height, elements;
	prefilterMap.GetDimensions(width, height, elements);

	uint3 coord = dispatchThreadID;
	if (coord.x >= width || coord.y >= height)
		return;

	float2 uv = (float2(coord.xy) + 0.5f) / float2(width, height);
	uv = uv * 2.0f - 1.0f;
	uv.y = -uv.y;

	// 面(Z)から法線方向(N)を計算
	float3 N;
	switch (coord.z)
	{
		case 0: N = float3(1.0f, uv.y, -uv.x); break;	// +X
		case 1: N = float3(-1.0f, uv.y, uv.x); break;	// -X
		case 2: N = float3(uv.x, 1.0f, -uv.y); break;	// +Y
		case 3: N = float3(uv.x, -1.0f, uv.y); break;	// -Y
		case 4: N = float3(uv.x, uv.y, 1.0f); break;	// +Z
		case 5: N = float3(-uv.x, uv.y, -1.0f); break;	// -Z
	}
	N = normalize(N);

	// V = R = N と仮定する
	float3 R = N;
	float3 V = R;

	const uint SAMPLE_COUNT = 1024u;
	float3 prefilteredColor = float3(0.0f, 0.0f, 0.0f);
	float totalWeight = 0.0f;
	
	for (uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		// Hammersley点群を用いてランダムかつ均等なサンプリング方向を生成
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, Roughness);
		float3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0f);
		if (NdotL > 0.0f)
		{
			float NdotH = max(dot(N, H), 0.0f);
			float HdotV = max(dot(H, V), 0.0f);
			float D = Roughness * Roughness * Roughness * Roughness; // GGX NDF
			float pdf = (D * NdotH) / (PI * pow(NdotH * NdotH * (D - 1.0f) + 1.0f, 2.0f) * HdotV + 0.0001f);
			
			float saTexel = 4.0f * PI / (6.0f * Resolution * Resolution);
			float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001f);
			
			float mipLevel = Roughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel); 

			prefilteredColor += environmentMap.SampleLevel(defaultSampler, L, mipLevel).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	if (totalWeight > 0.0f)
	{
		prefilteredColor = prefilteredColor / totalWeight;
	}

	prefilterMap[coord] = float4(prefilteredColor, 1.0f);
}
