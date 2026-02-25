// =========================================================================
// Span Engine - BRDF Integration LUT Generator
// =========================================================================

static const float PI = 3.14159265359f;

// 出力: 512x512 程度の 2D テクスチャ
RWTexture2D<float2> brdfLUT : register(u0);

// =========================================================================
// PBR Math Utilities (Hammersley Sequence & GGX)
// =========================================================================

float RadicalInverse_VdC(uint bits)
{
	return float(reversebits(bits)) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

// 重点サンプリング
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
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
	
	// 接空間からワールド空間へ
	float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	
	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

// 幾何減衰
float GeometrySchlickGGX_IBL(float NdotV, float roughness)
{
	float a = roughness;
	float k = (a * a) / 2.0f;

	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / denom;
}

float GeometrySmith_IBL(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx2 = GeometrySchlickGGX_IBL(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX_IBL(NdotL, roughness);

	return ggx1 * ggx2;
}

// =========================================================================
// Main Compute Shader
// =========================================================================

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint width, height;
	brdfLUT.GetDimensions(width, height);

	if (dispatchThreadID.x >= width || dispatchThreadID.y >= height)
		return;

	// X軸は NdotV (0.0 ～ 1.0)
	// Y軸は Roughness (0.0 ～ 1.0)
	float NdotV = (float(dispatchThreadID.x) + 0.5f) / float(width);
	float roughness = (float(dispatchThreadID.y) + 0.5f) / float(height);

	// 視線ベクトルを構築
	float3 V;
	V.x = sqrt(1.0f - NdotV * NdotV);
	V.y = 0.0f;
	V.z = NdotV;

	float A = 0.0f;
	float B = 0.0f;

	float3 N = float3(0.0f, 0.0f, 1.0f);

	const uint SAMPLE_COUNT = 1024u;
	for (uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		// 重点サンプリングでハーフベクトルを生成
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = max(L.z, 0.0f);
		float NdotH = max(H.z, 0.0f);
		float VdotH = max(dot(V, H), 0.0f);

		if (NdotL > 0.0f)
		{
			// 幾何減衰の計算
			float G = GeometrySmith_IBL(N, V, L, roughness);
			
			// 積分計算の重み
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0f - VdotH, 5.0f);

			A += (1.0f - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);

	// 結果をテクスチャに書き込み
	brdfLUT[dispatchThreadID.xy] = float2(A, B);
}
