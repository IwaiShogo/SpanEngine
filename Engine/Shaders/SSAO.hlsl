// =========================================================================
// Span Engine - SSAO (Screen Space Ambient Occlusion) Shader
// =========================================================================

cbuffer SSAOBuffer : register(b0)
{
	matrix Projection;
	matrix InvProjection;
	float4 Samples[64];	// 半球状のランダムサンプル (xyz: ベクトル, w: 未使用)
	float2 NoiseScale;	// 画面解像度 / ノイズテクスチャサイズ(4)
	float Radius;		// サンプリング半径
	float Bias;			// 自己遮蔽を防ぐバイアス
};

Texture2D t_GBuffer : register(t0);	// RGB: Normal, A: Linear Depth
Texture2D t_Noise : register(t1);	// RGB: Random Rotation Vector

SamplerState g_clampSampler : register(s0);	// GBuffer用
SamplerState g_wrapSampler : register(s1);	// ノイズテクスチャ用 (Wrap)

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

// =========================================================================
// Vertex Shader
// =========================================================================
PSInput VSMain(uint vertexID : SV_VertexID)
{
	PSInput output;
	output.uv = float2((vertexID << 1) & 2, vertexID & 2);
	output.position = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return output;
}

// 視点空間の深度とUVから、視点空間の3D座標を復元する関数
float3 GetViewPos(float2 uv, float linearDepth)
{
	// UVからクリップ空間座標へ変換
	float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, 1.0f, 1.0f);
	// 逆投影変換で視点空間のレイを計算
	float4 viewRay = mul(clipPos, InvProjection);
	viewRay.xyz /= viewRay.w;
	// レイを正規化し、リニアな深度を掛けて実際の視点座標を求める
	return normalize(viewRay.xyz) * linearDepth;
}

// =========================================================================
// Pixel Shader
// =========================================================================
float4 PSMain(PSInput input) : SV_TARGET
{
	// GBufferから法線と深度を取得
	float4 gbufferData = t_GBuffer.SampleLevel(g_clampSampler, input.uv, 0);
	float3 normal = normalize(gbufferData.rgb);
	float depth = gbufferData.a;

	// 背景の場合はSSAOを適用しない
	if (depth >= 9999.0f)
		return float4(1.0f, 1.0f, 1.0f, 1.0f);

	// 視点空間の3D座標を復元
	float3 fragPos = GetViewPos(input.uv, depth);

	// ノイズテクスチャからランダムな回転ベクトルを取得
	float3 randomVec = normalize(t_Noise.Sample(g_wrapSampler, input.uv * NoiseScale).xyz * 2.0f - 1.0f);

	// TBNマトリクスを構築
	float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	float3 bitangent = cross(normal, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, normal);

	float occlusion = 0.0f;
	const int kernelSize = 64;

	for (int i = 0; i < kernelSize; ++i)
	{
		// サンプルをView-Spaceに変換
		float3 samplePos = mul(Samples[i].xyz, TBN);
		samplePos = fragPos + samplePos * Radius;

		// サンプル座標を画面UVに投影して、その位置の深度を取得
		float4 offset = float4(samplePos, 1.0f);
		offset = mul(offset, Projection);
		offset.xyz /= offset.w;
		offset.xy = offset.xy * 0.5f + 0.5f;
		offset.y = 1.0f - offset.y;

		float sampleDepth = t_GBuffer.SampleLevel(g_clampSampler, offset.xy, 0).a;

		// 距離による重み付け
		float rangeCheck = smoothstep(0.0f, 1.0f, Radius / abs(fragPos.z - sampleDepth));
		
		// サンプルのZが、GBufferのZより奥にあれば遮蔽されていると判定
		occlusion += (sampleDepth <= samplePos.z - Bias ? 1.0f : 0.0f) * rangeCheck;
	}

	// 遮蔽率の平均化と反転
	occlusion = 1.0f - (occlusion / (float) kernelSize);

	return float4(occlusion, occlusion, occlusion, 1.0f);
}
