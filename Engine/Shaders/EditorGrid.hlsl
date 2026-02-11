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
	float2 uv : TEXCOORD;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float3 worldPos : POSITION;
	float2 uv : TEXCOORD;
};

// 頂点シェーダー
PS_INPUT VSMain(VS_INPUT input)
{
	PS_INPUT output;
	
	// グリッドはカメラのXZ位置に追従させる（無限に見せるため）
	// Y座標は0固定
	float3 pos = input.position;
	pos.x += cameraPos.x;
	pos.z += cameraPos.z;
	pos.y = 0.0f; // 床面固定

	// 行列変換
	float4 worldPos = float4(pos, 1.0f);
	output.position = mul(worldPos, mul(view, projection));
	output.worldPos = worldPos.xyz;
	output.uv = input.uv;
	
	return output;
}

// グリッド線を計算する関数
float GridFactor(float3 pos, float scale)
{
	float2 coord = pos.xz / scale;
	// 微分を使ってエイリアシングを防ぐ (太さを一定に見せる)
	float2 derivative = fwidth(coord);
	float2 grid = abs(frac(coord - 0.5) - 0.5) / derivative;
	float lineVal = min(grid.x, grid.y);
	return 1.0 - min(lineVal, 1.0);
}

// ピクセルシェーダー
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	float3 pos = input.worldPos;
	
	// --- グリッド設定 ---
	float thinScale = 1.0; // 1m
	float thickScale = 10.0; // 10m
	
	float4 thinColor = float4(0.35, 0.35, 0.35, 1.0); // 薄いグレー
	float4 thickColor = float4(0.1, 0.1, 0.1, 1.0); // 濃い黒 (背景より暗く)
	
	// グリッド線計算
	float thinGrid = GridFactor(pos, thinScale);
	float thickGrid = GridFactor(pos, thickScale);
	
	// --- 軸の描画 (X軸:赤, Z軸:青) ---
	float axisWidth = 0.05; // 軸の太さ
	float xAxis = step(abs(pos.z), axisWidth); // Zが0付近ならX軸
	float zAxis = step(abs(pos.x), axisWidth); // Xが0付近ならZ軸
	
	// ベースの色
	float4 color = float4(0, 0, 0, 0); // 透明
	
	// グリッド合成 (太線を優先)
	color = lerp(color, thinColor, thinGrid);
	color = lerp(color, thickColor, thickGrid);
	
	// 軸合成
	color = lerp(color, float4(1.0, 0.2, 0.2, 1.0), xAxis); // 赤
	color = lerp(color, float4(0.2, 0.2, 1.0, 1.0), zAxis); // 青
	
	// --- 距離フェード (遠くを消す) ---
	float dist = distance(input.worldPos.xz, cameraPos.xz);
	float fadeStart = 100.0;
	float fadeEnd = 1000.0; // ViewのFarClipに合わせて調整
	float alpha = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
	
	color.a *= alpha;
	
	// 完全に透明なら破棄 (深度バッファ汚染防止)
	if (color.a <= 0.01)
		discard;
	
	return color;
}
