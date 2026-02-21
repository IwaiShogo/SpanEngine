// =========================================================================
// Span Engine - Procedural Skybox Shader
// =========================================================================

cbuffer CameraBuffer : register(b0)
{
	matrix InverseView;
	matrix InverseProjection;
	float3 CameraPos;
	float Padding;
};

cbuffer SkyboxSettings : register(b1)
{
	float3 TopColor;
	float Padding1;
	float3 HorizonColor;
	float Padding2;
	float3 BottomColor;
	float Padding3;
};

struct VSOutput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
	float3 ViewRay : TEXCOORD1;
};

// --- Vertex Shader ---
VSOutput VSMain(uint id : SV_VertexID)
{
	VSOutput output;

	output.UV = float2((id << 1) & 2, id & 2);
	output.Position = float4(output.UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);

	float4 clipSpacePos = float4(output.Position.xy, 1.0f, 1.0f);
	float4 viewSpacePos = mul(clipSpacePos, InverseProjection);
	viewSpacePos.xyz /= viewSpacePos.w;

	output.ViewRay = mul(viewSpacePos.xyz, (float3x3) InverseView);

	return output;
}

// --- Pixel Shader ---
float4 PSMain(VSOutput input) : SV_TARGET
{
	// 視線方向を正規化
	float3 viewDir = normalize(input.ViewRay);

	// 高度(Y軸)に基づいて空の色をブレンドする (-1.0 ~ 1.0)
	float height = viewDir.y;

	float3 skyColor;

	// 少し指数をかけて、水平線付近のグラデーションを自然に寄せる
	if (height > 0.0f)
	{
		// 空 (水平線から上)
		float t = pow(height, 0.4f);
		skyColor = lerp(HorizonColor, TopColor, t);
	}
	else
	{
		// 地面 (水平線から下)
		float t = pow(-height, 0.4f);
		skyColor = lerp(HorizonColor, BottomColor, t);
	}

	return float4(skyColor, 1.0f);
}
