// スロット0: トランスフォーム (頂点ごと/オブジェクトごとに変わる)
cbuffer TransformBuffer : register(b0)
{
    matrix MVP;
    matrix World;
};

// スロット1: マテリアル (マテリアルごとに変わる)
cbuffer MaterialBuffer : register(b1)
{
    float3 Albedo;
    float Roughness;
    float Metallic;
    float Opacity;
    float3 Padding;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR; // 今回は使いませんが残しておきます
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD0; // ライト計算用のワールド座標
};

// --- Vertex Shader ---
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), MVP);
    
    // 法線の回転 (正規化を忘れずに)
    output.normal = normalize(mul(input.normal, (float3x3) World));
    
    // ワールド座標
    output.worldPos = mul(float4(input.position, 1.0f), World).xyz;
    
    return output;
}

// --- Pixel Shader ---
float4 PSMain(PSInput input) : SV_TARGET
{
    // 1. 基本ベクトル
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float3 viewDir = normalize(float3(0.0f, 2.0f, -5.0f) - input.worldPos);
    float3 normal = normalize(input.normal);
    float3 halfVector = normalize(lightDir + viewDir);

    // 2. 拡散反射 (Diffuse)
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = Albedo * NdotL;

    // 3. 鏡面反射 (Specular)
    // ガラスは鋭いハイライトを持つので、光沢度(Shininess)を高く保つ
    float shininess = (1.0f - Roughness) * 256.0f; // 128 -> 256 に強化
    float NdotH = max(dot(normal, halfVector), 0.0f);
    float specIntensity = pow(NdotH, max(shininess, 1.0f));
    
    // 金属度が高い場合、スペキュラに色が乗る
    float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
    float3 finalSpecular = specularColor * specIntensity;

    // 4. 環境光
    float3 ambient = Albedo * 0.1f;

    // ===========================================================
    // フレネル効果
    // ===========================================================
    // 視線と法線の角度 (1.0=正面、0.0=真横)
    float NdotV = max(dot(normal, viewDir), 0.0f);
    
    // 縁に行くほど強くなる係数 (Schlick近似の応用)
    float fresnel = pow(1.0f - NdotV, 3.0f);

    // ガラス表現のトリック:
    // 1. 縁に行くほど反射(スペキュラ)を強くする
    finalSpecular += fresnel * 0.5f;

    // 2. 縁に行くほど不透明にする (Opacity + Fresnel)
    // これにより「中心は透けてるけど、輪郭はくっきり」したガラス玉になります
    float finalAlpha = saturate(Opacity + fresnel);

    // ===========================================================

    // 合成
    float3 finalColor = ambient + (diffuse * (1.0f - Metallic)) + finalSpecular;

    return float4(finalColor, finalAlpha);
}