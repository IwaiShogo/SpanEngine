// =========================================================================
// Span Engine - Skybox Shader
// =========================================================================

cbuffer SkyboxCameraCB : register(b0)
{
	matrix InvView;
	matrix InvProj;
	float3 CamPos;
	float padding1;
};

cbuffer SkyboxSettingsCB : register(b1)
{
	float3 TopColor;
	float pad1;
	float3 HorizonColor;
	float pad2;
	float3 BottomColor;
	int SkyMode; // 0: Procedural(3êF), 1: HDRI
	float Exposure;
	float3 pad3;
};

TextureCube t_EnvironmentMap : register(t0);
SamplerState s_linear : register(s0);

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 viewDir : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
	VSOutput output;
    
	float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    
    // UV Ç NDCç¿ïW Ç…ïœä∑
	float4 ndcPos = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);
	output.position = ndcPos;

	float4 viewPos = mul(ndcPos, InvProj);
	viewPos.xyz /= viewPos.w;
	float4 worldPos = mul(float4(viewPos.xyz, 0.0f), InvView);
    
	output.viewDir = worldPos.xyz;

	return output;
}

float3 ACESFilm(float3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	float3 dir = normalize(input.viewDir);
	float3 color = float3(0, 0, 0);

	if (SkyMode == 0) // Procedural 3-Color
	{
		float height = dir.y;
		if (height > 0.0f)
			color = lerp(HorizonColor, TopColor, pow(max(height, 0.0001f), 0.4f));
		else
			color = lerp(HorizonColor, BottomColor, pow(max(-height, 0.0001f), 0.4f));
            
		color *= Exposure;
	}
	else // HDRI Cubemap
	{
		color = t_EnvironmentMap.SampleLevel(s_linear, dir, 0).rgb;
		color *= Exposure;
	}

	color = ACESFilm(color);
	color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

	return float4(color, 1.0f);
}
