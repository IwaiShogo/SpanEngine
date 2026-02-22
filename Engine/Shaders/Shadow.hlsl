// =========================================================================
// Span Engine - Shadow Pass Shader
// =========================================================================

cbuffer TransformBuffer : register(b0)
{
	matrix MVP;
	matrix World;
};

struct VSInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float2 uv       : TEXCOORD0;
};

// 頂点シェーダー
float4 VSMain(VSInput input) : SV_POSITION
{
	return mul(float4(input.position, 1.0f), MVP);
}
