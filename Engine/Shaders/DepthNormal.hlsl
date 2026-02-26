// =========================================================================
// Span Engine - Depth & Normal Pre-pass Shader
// =========================================================================

cbuffer PassCB : register(b0)
{
	matrix MVP;
	matrix World;
	matrix View;
};

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 normalVS : NORMAL;
	float viewZ : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
	PSInput output;

	// クリップ空間の座標
	output.position = mul(float4(input.position, 1.0f), MVP);

	// ワールド法線を計算してから、視点空間の法線に変換
	float3 worldNormal = mul(input.normal, (float3x3)World);
	output.normalVS = mul(worldNormal, (float3x3)View);

	// 視点空間の座標を計算し、Z値を取得
	float4 viewPos = mul(mul(float4(input.position, 1.0f), World), View);
	output.viewZ = viewPos.z;

	return output;
}

float PSMain(PSInput input) : SV_TARGET
{
	// 法線の正規化
	float3 N = normalize(input.normalVS);

	return float4(N, input.viewZ);
}
