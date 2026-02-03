cbuffer TransformBuffer : register(b0)
{
	matrix MVP;
	matrix World;
};

cbuffer MaterialBuffer : register(b1)
{
	float3 Albedo;
	float Roughness;
	float Metallic;
	float Opacity;
	float UseTexture;
	float Padding;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 worldPos : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

// --- Vertex Shader ---
PSInput VSMain(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1.0f), MVP);

	output.normal = normalize(mul(input.normal, (float3x3) World));

	output.worldPos = mul(float4(input.position, 1.0f), World).xyz;

	output.uv = input.uv;

	return output;
}

// --- Pixel Shader ---
float4 PSMain(PSInput input) : SV_TARGET
{
	float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
	float3 viewDir = normalize(float3(0.0f, 2.0f, -5.0f) - input.worldPos);
	float3 normal = normalize(input.normal);
	float3 halfVector = normalize(lightDir + viewDir);

	float3 baseColor = Albedo;
	if (UseTexture > 0.5f)
	{
		float4 texColor = g_texture.Sample(g_sampler, input.uv);
		baseColor *= texColor.rgb;
	}

	float NdotL = max(dot(normal, lightDir), 0.0f);
	float3 diffuse = baseColor * NdotL;

	float shininess = (1.0f - Roughness) * 256.0f;
	float NdotH = max(dot(normal, halfVector), 0.0f);
	float specIntensity = pow(NdotH, max(shininess, 1.0f));

	float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, Metallic);
	float3 finalSpecular = specularColor * specIntensity;

	float3 ambient = baseColor * 0.1f;

	float NdotV = max(dot(normal, viewDir), 0.0f);

	float fresnel = pow(1.0f - NdotV, 3.0f);

	finalSpecular += fresnel * 0.5f;

	float finalAlpha = saturate(Opacity + fresnel);

	float3 finalColor = ambient + (diffuse * (1.0f - Metallic)) + finalSpecular;

	return float4(finalColor, finalAlpha);
}
