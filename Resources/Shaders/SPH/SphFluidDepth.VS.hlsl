// SphFluidDepth.VS.hlsl — パーティクルをカメラ正対 billboard quad に展開する
// 各インスタンスが 6 頂点 (2 三角形) を生成する

struct SphInstance {
    float4x4 WVP;
    float4x4 World;
    float4   color;
};
StructuredBuffer<SphInstance> g_Instances : register(t0);

cbuffer FluidDepthParams : register(b0) {
    float4x4 g_View;        // ワールド → ビュー
    float4x4 g_Proj;        // ビュー   → クリップ
    float2   g_Resolution;
    float    _pad0, _pad1;
};

struct VSOut {
    float4 pos      : SV_Position;
    float3 centerVS : TEXCOORD0;   // ビュー空間の球中心
    float2 quadUV   : TEXCOORD1;   // -1..1 (billboard 正規化 UV)
    float  radius   : TEXCOORD2;
};

// 6 頂点 / quad (三角形 × 2)
static const float2 kUVs[6] = {
    {-1.0f,-1.0f}, { 1.0f,-1.0f}, {-1.0f, 1.0f},
    { 1.0f,-1.0f}, { 1.0f, 1.0f}, {-1.0f, 1.0f},
};

VSOut main(uint vid : SV_VertexID, uint iid : SV_InstanceID) {
    SphInstance inst = g_Instances[iid];

    // World 行列 Row 3 = 平行移動 (エンジンの行ベクトル規約: mul(world, viewProj))
    float3 worldPos = inst.World[3].xyz;
    // World 行列 Row 0, 列 0 = Scale.x = particleRadius
    float  radius   = inst.World[0].x;

    // ビュー空間へ変換
    float3 centerVS = mul(float4(worldPos, 1.0f), g_View).xyz;

    // ビュー空間 XY に billboard コーナーを展開
    float2 uv       = kUVs[vid % 6];
    float3 cornerVS = centerVS + float3(uv.x * radius, uv.y * radius, 0.0f);

    VSOut o;
    o.pos      = mul(float4(cornerVS, 1.0f), g_Proj);
    o.centerVS = centerVS;
    o.quadUV   = uv;
    o.radius   = radius;
    return o;
}
