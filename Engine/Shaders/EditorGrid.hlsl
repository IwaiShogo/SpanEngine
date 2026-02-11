// EditorGrid.hlsl

cbuffer SceneConstantBuffer : register(b0)
{
	matrix view;
	matrix projection;
	float3 cameraPos;
	float padding;
};

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float3 worldPos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

// 頂点シェーダー
PS_INPUT VSMain(VS_INPUT input)
{
	PS_INPUT output;
	
	output.normal = input.normal;
	output.uv = input.uv;

	// 法線のYが上を向いている(>0.5)なら「無限グリッドの床」として扱う
	// それ以外（横向き）なら「Y軸の線」として扱う
	if (input.normal.y > 0.5f)
	{
		// --- 床（無限グリッド） ---
		float3 pos = input.position;
		pos.x += cameraPos.x;
		pos.z += cameraPos.z;
		pos.y = 0.0f; // 床面固定

		float4 worldPos = float4(pos, 1.0f);
		output.position = mul(worldPos, mul(view, projection));
		output.worldPos = worldPos.xyz;
	}
	else
	{
		// --- Y軸（固定オブジェクト） ---
		float4 worldPos = float4(input.position, 1.0f);
		output.position = mul(worldPos, mul(view, projection));
		output.worldPos = worldPos.xyz;
	}
	
	return output;
}

// グリッド線を計算する関数
float GridFactor(float3 pos, float scale)
{
	float2 coord = pos.xz / scale;
	float2 derivative = fwidth(coord);
	float2 grid = abs(frac(coord - 0.5) - 0.5) / derivative;
	float lineVal = min(grid.x, grid.y);
	return 1.0 - min(lineVal, 1.0);
}

// ピクセルシェーダー
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	// Y軸の描画判定（法線が横を向いている場合）
	if (input.normal.y < 0.5f)
	{
		// 緑色 (Y軸)
		return float4(0.2, 1.0, 0.2, 1.0);
	}

	// --- 以下、床のグリッド描画 ---

	float3 pos = input.worldPos;
	
	// 設定: 暗い背景に白い線
	float thinScale = 1.0; // 1m
	float thickScale = 10.0; // 10m
	
	// 「控えめな細い線」と「明確な太い線」
	float4 thinColor = float4(0.7, 0.7, 0.7, 0.15); // かなり薄く透明に
	float4 thickColor = float4(0.8, 0.8, 0.8, 0.4); // 少し明るく、濃いめに
	
	float thinGrid = GridFactor(pos, thinScale);
	float thickGrid = GridFactor(pos, thickScale);
	
	// --- 軸の描画 (X軸:赤, Z軸:青) ---
	float axisWidth = 0.05;
	float xAxis = step(abs(pos.z), axisWidth);
	float zAxis = step(abs(pos.x), axisWidth);
	
	float4 color = float4(0, 0, 0, 0); // ベースは透明
	
	// グリッド合成
	color = lerp(color, thinColor, thinGrid);
	color = lerp(color, thickColor, thickGrid);
	
	// 軸合成
	color = lerp(color, float4(1.0, 0.1, 0.1, 1.0), xAxis); // 赤 (X)
	color = lerp(color, float4(0.1, 0.1, 1.0, 1.0), zAxis); // 青 (Z)
	
	// --- 距離フェード (遠くを消す) ---
	float dist = distance(input.worldPos.xz, cameraPos.xz);
	float fadeStart = 50.0; // 少し手前から消し始める
	float fadeEnd = 500.0;
	
	float alpha = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
	color.a *= alpha;
	
	// 透明なら破棄
	if (color.a <= 0.001)
		discard;
	
	return color;
}
