// CsPrepareInstances.hlsl
// 粒子位置を読んで WVP 行列と色をインスタンシングバッファに書き込む
// → CPU→GPU コピーが不要になり DrawInstanced まで全処理を GPU で完結
#include "SphCommon.hlsli"

RWStructuredBuffer<SphParticle> g_Particles : register(u0);
RWStructuredBuffer<SphInstance> g_Instances : register(u1);

// スケール行列 (対角成分のみ)
float4x4 MakeScale(float s) {
    return float4x4(
        s, 0, 0, 0,
        0, s, 0, 0,
        0, 0, s, 0,
        0, 0, 0, 1);
}

// 平行移動行列
float4x4 MakeTranslate(float3 t) {
    return float4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        t.x, t.y, t.z, 1);
}

[numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint i = tid.x;
    if ((int)i >= g_ParticleCount) return;

    float3 pos   = g_Particles[i].position;
    float3 vel   = g_Particles[i].velocity;
    float  speed = length(vel);
    float  t     = saturate(speed / g_SpeedMax);

    // 速度に応じて色補間
    float4 col = lerp(g_ColorLow, g_ColorHigh, t);

    // World = Scale × Translate (球は回転不要)
    float4x4 scale     = MakeScale(g_ParticleRadius);
    float4x4 translate = MakeTranslate(pos);
    float4x4 world     = mul(scale, translate);
    float4x4 wvp       = mul(world, g_ViewProj);

    g_Instances[i].WVP   = wvp;
    g_Instances[i].World = world;
    g_Instances[i].color = col;
}
