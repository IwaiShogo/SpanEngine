// =========================================================================
// Span Engine - Forward+ Light Culling Compute Shader
// =========================================================================

// タイルのサイズ (16 x 16 ピクセル)
#define TILE_SIZE 16

// 1つのタイルが持つフラスタムの4つの平面
struct Frustum
{
	float4 planes[4];	// Left, Right, Top, Bottom
};

// C++ から渡される定数
cbuffer CullingCB : register(b0)
{
	matrix InverseProjection;	// 画面座標から視点空間へ戻すための行列
	matrix View;				// ワールド座標から視点空間へ返還するための行列
	uint2 ScreenDimensions;		// 画面の解像度 (幅, 高さ)
	uint2 NumTiles;				// タイルの数 (X, Y)
};

// 全タイルのフラスタムを保存するバッファ (UAV)
RWStructuredBuffer<Frustum> outFrustums : register(u0);

// Helper Functions
// =========================================================================

// 画面空間の座標を視点空間レイに変換する
float3 ScreenToView(float4 screenPos)
{
	float4 viewPos = mul(screenPos, InverseProjection);
	return viewPos.xyz / viewPos.w;
}

// 3つの点から変面の方程式を計算する
float4 ComputePlane(float3 p0, float3 p1, float3 p2)
{
	float3 v0 = p1 - p0;
	float3 v2 = p2 - p0;
	float3 normal = normalize(cross(v0, v2));
	float distance = -dot(normal, p0);
	return float4(normal, distance);
}

// CS_ComputeFrustums
// =========================================================================
// 1つのグループで16x16=256スレッドではなく、1スレッドが1タイルを担当する設計にします。
// Groupサイズは 16x16 とし、256個のタイルをまとめて処理します。

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CS_ComputeFrustums(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	// 現在のスレッドIDがタイルのインデックス
	uint2 tileXY = dispatchThreadId.xy;

	// 画面外のタイルならスキップ
	if (tileXY.x >= NumTiles.x || tileXY.y >= NumTiles.y)
		return;

	// タイルの4つの角のスクリーン座標を計算
	float2 invScreenDim = 1.0f / float2(ScreenDimensions);
	float2 step = float2(TILE_SIZE, TILE_SIZE) * invScreenDim * 2.0f;

	float2 topLeft = float2(tileXY) * step + float2(-1.0f, -1.0f);
	float2 bottomRight = topLeft + step;
	
	// Y軸反転
	topLeft.y = -topLeft.y;
	bottomRight.y = -bottomRight.y;

	float2 bottomLeft = float2(topLeft.x, bottomRight.y);
	float2 topRight = float2(bottomRight.x, topLeft.y);

	// 視点空間での4つの角へのレイを計算。深度は1.0と仮定
	float3 viewTopLeft = ScreenToView(float4(topLeft, 1.0f, 1.0f));
	float3 viewBottomRight = ScreenToView(float4(bottomRight, 1.0f, 1.0f));
	float3 viewBottomLeft = ScreenToView(float4(bottomLeft, 1.0f, 1.0f));
	float3 viewTopRight = ScreenToView(float4(topRight, 1.0f, 1.0f));

	// カメラの原点
	float3 eyePos = float3(0.0f, 0.0f, 0.0f);

	// 4つの平面を計算
	Frustum frustum;
	// Left Plane (eye -> TopLeft -> BottomLeft)
	frustum.planes[0] = ComputePlane(eyePos, viewBottomLeft, viewTopLeft);
	// Right Plane (eye -> BottomRight -> TopRight)
	frustum.planes[1] = ComputePlane(eyePos, viewTopRight, viewBottomRight);
	// Top Plane (eye -> TopLeft -> TopRight)
	frustum.planes[2] = ComputePlane(eyePos, viewTopLeft, viewTopRight);
	// Bottom Plane (eye -> BottomRight -> BottomLeft)
	frustum.planes[3] = ComputePlane(eyePos, viewBottomRight, viewBottomLeft);

	// バッファに書き込み
	uint tileIndex = tileXY.y * NumTiles.x + tileXY.x;
	outFrustums[tileIndex] = frustum;
}

// ライトの最大数
#define MAX_LIGHTS 1024
#define MAX_LIGHTS_PER_TILE 256

struct LightData
{
	float3 Color;
	float Intensity;
	float3 Position;
	float Range;
	float3 Direction;
	int Type;
	float InnerConeAngle;
	float OuterConeAngle;
	int CastShadows;
	int ShadowIndex;
	row_major matrix ShadowMatrix;
};

// C++ から渡されるライトデータ
cbuffer LightBuffer : register(b1)
{
	float3 CameraPosition;
	float Exposure;
	float3 SkyTopColor;
	float AmbientIntensity;
	float3 SkyHorizonColor;
	float EnvReflectionIntensity;
	float3 SkyBottomColor;
	int ActiveLightCount;
	int SkyMode;
	int EnableSSAO;
	float2 padding_lb;
	Matrix DirectionalLightSpaceMatrix;
};

StructuredBuffer<LightData> Lights : register(t0);

// u1: Opaque Lights Index Counter
RWStructuredBuffer<uint> outLightIndexCounter : register(u1);
// u2: Opaque Light Index List
RWStructuredBuffer<uint> outLightIndexList : register(u2);
// u3: Light Grid
RWStructuredBuffer<uint2> outLightGrid : register(u3);

// GroupShared メモリ
groupshared uint s_MinZ;
groupshared uint s_MaxZ;
groupshared uint s_TileLightCount;
groupshared uint s_TileLightIndices[MAX_LIGHTS_PER_TILE];

// 視点空間の深度をuintに変換
uint AsUint(float v) { return asuint(v); }
float AsFloat(uint v) { return asfloat(v); }

// CS_LightCulling
// =========================================================================

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CS_LightCulling(
	uint3 groupId : SV_GroupID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint groupIndex : SV_GroupIndex)
{
	// スレッド0が共有メモリを初期化
	if (groupIndex == 0)
	{
		s_TileLightCount = 0;
	}

	GroupMemoryBarrierWithGroupSync();

	// タイルのインデックスを計算
	uint tileIndex = groupId.y * NumTiles.x + groupId.x;
	
	// タイルのフラスタムを取得
	Frustum frustum = outFrustums[tileIndex];

	// 各スレッドが、シーン内のライトを順番に判定する
	for (uint i = groupIndex; i < ActiveLightCount; i += TILE_SIZE * TILE_SIZE)
	{
		LightData light = Lights[i];
		bool inFrustum = true;

		// Directional Light は常に全タイルに影響する
		if (light.Type != 0)
		{
			// Point / Spot Light の場合、視点空間での位置を計算
			float4 viewPos = mul(float4(light.Position, 1.0f), View);
			viewPos.xyz /= viewPos.w;

			// フラスタムの4つの平面と球(ライトのRange)の交差判定
			for (int p = 0; p < 4; ++p)
			{
				float distance = dot(frustum.planes[p].xyz, viewPos.xyz) + frustum.planes[p].w;
				// 球が平面の完全に「外側」にある場合
				if (distance < -light.Range)
				{
					inFrustum = false;
					break;
				}
			}
		}

		// フラスタム内に入っていれば、共有メモリのリストに追加
		if (inFrustum)
		{
			uint index;
			InterlockedAdd(s_TileLightCount, 1, index);
			if (index < MAX_LIGHTS_PER_TILE)
			{
				s_TileLightIndices[index] = i;
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0)
	{
		// 実際のライト数を最大値でクリップ
		uint lightCount = min(s_TileLightCount, MAX_LIGHTS_PER_TILE);

		uint offset;
		InterlockedAdd(outLightIndexCounter[0], lightCount, offset);

		// Gridに「オフセット」と「ライト数」を記録
		outLightGrid[tileIndex] = uint2(offset, lightCount);

		// Listにライトのインデックスを書き込む
		for (uint j = 0; j < lightCount; ++j)
		{
			outLightIndexList[offset + j] = s_TileLightIndices[j];
		}
	}
}

// CS_ResetCounter
// =========================================================================
[numthreads(1, 1, 1)]
void CS_ResetCounter()
{
	outLightIndexCounter[0] = 0;
}
