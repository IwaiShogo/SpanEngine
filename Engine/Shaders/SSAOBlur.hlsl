// =========================================================================
// Span Engine - SSAO Blur Shader
// =========================================================================

Texture2D t_SSAOMap : register(t0);
SamplerState g_clampSampler : register(s0);

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

// =========================================================================
// Vertex Shader (Fullscreen Triangle)
// =========================================================================
PSInput VSMain(uint vertexID : SV_VertexID)
{
	PSInput output;
	output.uv = float2((vertexID << 1) & 2, vertexID & 2);
	output.position = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return output;
}

// =========================================================================
// Pixel Shader
// =========================================================================
float4 PSMain(PSInput input) : SV_TARGET
{
	// 1ピクセルのUVサイズを取得
	float2 texelSize;
	float width, height;
	t_SSAOMap.GetDimensions(width, height);
	texelSize = float2(1.0f / width, 1.0f / height);

	float result = 0.0f;

	// 4x4 ブロックの平均をとってノイズをぼかす
	for (int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y)
		{
			float2 offset = float2(float(x), float(y)) * texelSize;
			result += t_SSAOMap.SampleLevel(g_clampSampler, input.uv + offset, 0).r;
		}
	}
	
	result = result / 16.0f;
	return float4(result, result, result, 1.0f);
}
