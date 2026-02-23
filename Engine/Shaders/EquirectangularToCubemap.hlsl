// =========================================================================
// Span Engine - Equirectangular to Cubemap
// =========================================================================

// 入力: パノラマHDR画像 (SRV)
Texture2D inputTexture : register(t0);
SamplerState defaultSampler : register(s0);

// 出力: Cubemap
RWTexture2DArray<float4> outputCubemap : register(u0);

// 定数
static const float2 invAtan = float2(0.1591, 0.3183);	// 1 / (2 * PI), 1 / PI

// 3Dベクトルからパノラマ画像のUV座標に変換する
float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

// スレッドグループのサイズ
[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	// 出力先Cubemapの解像度を取得
	uint width, height, elements;
	outputCubemap.GetDimensions(width, height, elements);

	uint3 coord = dispatchThreadID;
	if (coord.x >= width || coord.y >= height) return;

	// ピクセル座標を -1.0 ~ 1.0 に正規化
	float2 uv = (float2(coord.xy) + 0.5f) / float2(width, height);
	uv = uv * 2.0f - 1.0f;

	float3 dir;
	// スライス(面)毎に、3D空間上の方向ベクトルを計算
	switch (coord.z)
	{
		case 0: dir = float3( 1.0f, -uv.y, -uv.x); break; // +X
		case 1: dir = float3(-1.0f, -uv.y,	uv.x); break; // -X
		case 2: dir = float3( uv.x,	 1.0f,	uv.y); break; // +Y
		case 3: dir = float3( uv.x, -1.0f, -uv.y); break; // -Y
		case 4: dir = float3( uv.x, -uv.y,	1.0f); break; // +Z
		case 5: dir = float3(-uv.x, -uv.y, -1.0f); break; // -Z
	}

	dir = normalize(dir);

	// 方向ベクトルを使って、パノラマ画像から色を取得
	float2 sphericalUV = SampleSphericalMap(dir);
	sphericalUV.y = 1.0f - sphericalUV.y;
	float3 color = inputTexture.SampleLevel(defaultSampler, sphericalUV, 0).rgb;

	// Cubemapに書き込み
	outputCubemap[coord] = float4(color, 1.0f);
}
