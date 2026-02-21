// =========================================================================
// Span Engine - Standard PBR Shader
// =========================================================================

cbuffer TransformBuffer : register(b0)
{
	matrix MVP;
	matrix World;
};

// C++側の MaterialData 構造体と完全に一致させること (16バイト境界)
cbuffer MaterialBuffer : register(b1)
{
	float4 AlbedoColor;
	float3 EmissiveColor;
	float Roughness;

	float Metallic;
	float AO;
	float Cutoff;
	float Padding1;

	float2 Tiling;
	float2 Offset;

	int HasAlbedoMap;
	int HasNormalMap;
	int HasMetallicMap;
	int HasRoughnessMap;

	int HasAOMap;
	int HasEmissiveMap;
	int Padding2[2];
};

// テクスチャとサンプラー (順序はRenderer.cppのDescriptorTableへのセット順に依存)
// 今回はRenderer側で「テクスチャの配列(Table)」として渡す設計に変更します。
Texture2D t_Albedo : register(t0);
Texture2D t_Normal : register(t1);
Texture2D t_Metallic : register(t2);
Texture2D t_Roughness : register(t3);
Texture2D t_AO : register(t4);
Texture2D t_Emissive : register(t5);

SamplerState g_sampler : register(s0);

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
	// ※必要に応じて TANGENT などを追加
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 worldPos : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

// =========================================================================
// Constants & PBR Functions
// =========================================================================
static const float PI = 3.14159265359;

// Trowbridge-Reitz GGX (法線分布関数)
float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / max(denom, 0.001);
}

// Schlick-GGX (幾何減衰関数)
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

// Smith (幾何減衰の合成)
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

// Fresnel-Schlick (フレネル反射)
float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// =========================================================================
// Vertex Shader
// =========================================================================
PSInput VSMain(VSInput input)
{
	PSInput output;

	output.position = mul(float4(input.position, 1.0f), MVP);
	output.normal = normalize(mul(input.normal, (float3x3) World));
	output.worldPos = mul(float4(input.position, 1.0f), World).xyz;

	// UVのタイリングとオフセットを適用
	output.uv = (input.uv * Tiling) + Offset;

	return output;
}

// =========================================================================
// Pixel Shader
// =========================================================================
float4 PSMain(PSInput input) : SV_TARGET
{
	// --- 1. テクスチャのサンプリング ---
	float4 albedo = AlbedoColor;
	if (HasAlbedoMap)
	{
		albedo *= t_Albedo.Sample(g_sampler, input.uv);
	}

	// Cutout (Alpha Test)
	if (albedo.a < Cutoff)
		discard;

	float meta = Metallic;
	if (HasMetallicMap)
		meta *= t_Metallic.Sample(g_sampler, input.uv).r;

	float rough = Roughness;
	if (HasRoughnessMap)
		rough *= t_Roughness.Sample(g_sampler, input.uv).r;

	float aoMap = AO;
	if (HasAOMap)
		aoMap *= t_AO.Sample(g_sampler, input.uv).r;

	float3 emissive = EmissiveColor;
	if (HasEmissiveMap)
		emissive += t_Emissive.Sample(g_sampler, input.uv).rgb;

	// 法線マップの処理
	float3 N = normalize(input.normal);
	if (HasNormalMap)
	{
		// マップから法線を取得 (0~1 -> -1~1)
		float3 tangentNormal = t_Normal.Sample(g_sampler, input.uv).xyz * 2.0 - 1.0;
		tangentNormal.y = -tangentNormal.y;	// Y(G)チェンネルの反転補正

		// 偏微分による接ベクトル空間の計算
		float3 q1  = ddx(input.worldPos);
		float3 q2  = ddy(input.worldPos);
		float2 st1 = ddx(input.uv);
		float2 st2 = ddy(input.uv);

		float3 T = normalize(q1 * st2.y - q2 * st1.y);
		float3 B = -normalize(cross(N, T));

		float3x3 TBN = float3x3(T, B, N);
		N = normalize(mul(tangentNormal, TBN));
	}

	// --- 2. PBR Lighting Setup ---
	float3 V = normalize(float3(0.0f, 2.0f, -5.0f) - input.worldPos); // カメラ位置 (本来はCBから渡す)

	// F0 (基本反射率): 非金属は0.04、金属はAlbedoの色を使用
	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo.rgb, meta);

	float3 Lo = float3(0.0, 0.0, 0.0);

	// --- 3. Directional Light 計算 (仮の単一光源) ---
	float3 LDir = normalize(float3(1.0, 1.0, -1.0));
	float3 LColor = float3(1.0, 1.0, 1.0) * 3.0; // 光の強さ

	float3 H = normalize(V + LDir);
	float NdotL = max(dot(N, LDir), 0.0);

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(N, H, rough);
	float G = GeometrySmith(N, V, LDir, rough);
	float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

	float3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001; // 0除算防止
	float3 specular = numerator / denominator;

	// エネルギー保存の法則
	float3 kS = F;
	float3 kD = float3(1.0, 1.0, 1.0) - kS;
	kD *= 1.0 - meta;

	// 出力される光の量
	Lo += (kD * albedo.rgb / PI + specular) * LColor * NdotL;

	// --- 4. Ambient & Final Color ---
	float3 ambient = float3(0.03, 0.03, 0.03) * albedo.rgb * aoMap; // 簡易環境光
	float3 color = ambient + Lo + emissive;

	// HDR Tone Mapping (Reinhard) & Gamma Correction
	color = color / (color + float3(1.0, 1.0, 1.0));
	color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

	return float4(color, albedo.a);
}
