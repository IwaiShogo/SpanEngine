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
    // ライティング設定
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f)); // 平行光源
    float3 viewDir = normalize(float3(0.0f, 2.0f, -5.0f) - input.worldPos); // カメラ方向(仮)
    float3 normal = normalize(input.normal);

    // 1. 拡散反射 (Diffuse) - Lambert
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = Albedo * NdotL;

    // 2. 鏡面反射 (Specular) - Blinn-Phong (簡易的なPBR風)
    float3 halfVector = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfVector), 0.0f);
    
    // 粗さ(Roughness)から光沢度(Shininess)を計算
    // Roughness: 0(ツルツル) -> Shininess: 高い
    // Roughness: 1(ザラザラ) -> Shininess: 低い
    float shininess = (1.0f - Roughness) * 128.0f;
    float specIntensity = pow(NdotH, max(shininess, 1.0f));
    
    // 金属度(Metallic)が高いと、拡散反射が減り、スペキュラに色が付く
    float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
    float3 finalSpecular = specularColor * specIntensity;

    // 3. 環境光
    float3 ambient = Albedo * 0.1f;

    // 合成
    // 金属は拡散反射成分を持たない（すべて反射する）
    float3 finalColor = ambient + (diffuse * (1.0f - Metallic)) + finalSpecular;

    return float4(finalColor, 1.0f);
}