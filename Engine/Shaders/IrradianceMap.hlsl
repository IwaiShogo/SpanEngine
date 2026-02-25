// =========================================================================
// Span Engine - Irradiance Map Generator (Compute Shader)
// =========================================================================

static const float PI = 3.14159265359f;

// 入力: 生成済みの環境Cubemap
TextureCube environmentMap : register(t0);
SamplerState defaultSampler : register(s0);

// 出力: Irradiance用Cubemap (解像度32x32程度)
RWTexture2DArray<float4> irradianceMap : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint width, height, elements;
	irradianceMap.GetDimensions(width, height, elements);

	uint3 coord = dispatchThreadID;
	if (coord.x >= width || coord.y >= height)
		return;

	// UV座標の計算
	float2 uv = (float2(coord.xy) + 0.5f) / float2(width, height);
	uv = uv * 2.0f - 1.0f;
	uv.y = -uv.y; // Y反転

	// 面(Z)から法線方向(N)を計算
	float3 N;
	switch (coord.z)
	{
		case 0:
			N = float3(1.0f, uv.y, -uv.x);
			break; // +X
		case 1:
			N = float3(-1.0f, uv.y, uv.x);
			break; // -X
		case 2:
			N = float3(uv.x, 1.0f, -uv.y);
			break; // +Y
		case 3:
			N = float3(uv.x, -1.0f, uv.y);
			break; // -Y
		case 4:
			N = float3(uv.x, uv.y, 1.0f);
			break; // +Z
		case 5:
			N = float3(-uv.x, uv.y, -1.0f);
			break; // -Z
	}
	N = normalize(N);

	// 接空間の基底ベクトルを作成
	float3 up = abs(N.y) < 0.999f ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
	float3 right = normalize(cross(up, N));
	up = cross(N, right);

	// --- 畳み込み積分 ---
	float3 irradiance = float3(0.0, 0.0, 0.0);
	
	// サンプル密度
	float sampleDelta = 0.025f;
	float nrSamples = 0.0f;

	for (float phi = 0.0f; phi < 2.0f * PI; phi += sampleDelta)
	{
		for (float theta = 0.0f; theta < 0.5f * PI; theta += sampleDelta)
		{
			// 球面座標からデカルト座標へ
			float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			
			// 接空間からワールド空間へ
			float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			// 環境マップから色を取得
			float3 sampleColor = environmentMap.SampleLevel(defaultSampler, sampleVec, 0).rgb;

			// ホタル除去
			float hdrMax = 10.0f;
			float luminance = max(max(sampleColor.r, sampleColor.g), sampleColor.b);
			if (luminance > hdrMax)
			{
				sampleColor *= (hdrMax / luminance);
			}
			
			irradiance += sampleColor * cos(theta) * sin(theta);
			nrSamples++;
		}
	}

	irradiance = PI * irradiance * (1.0f / float(nrSamples));
	
	// 書き込み
	irradianceMap[coord] = float4(irradiance, 1.0f);
}
