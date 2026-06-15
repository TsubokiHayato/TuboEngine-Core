// SphFluidBlur.PS.hlsl — バイラテラルブラー (エッジ保持) を流体深度テクスチャに適用
// 隣接粒子の境界を滑らかに繋ぎ、大きな深度差 (粒子間の隙間) は跨がない

#include "../PostEffect/CopyImage.hlsli"

cbuffer BlurParams : register(b0) {
    float2 g_Resolution;
    float  g_BlurRadius;    // ブラーカーネル半径 (1-5 テクセル)
    float  g_DepthFalloff;  // 深度差重み係数 (大きいほどエッジ保持が強い)
};

Texture2D<float> g_Depth   : register(t0);
SamplerState     g_Sampler : register(s0);

float main(VertexShaderOutput input) : SV_Target0 {
    float2 uv   = input.texcoord;
    float2 step = float2(1.0f / g_Resolution.x, 1.0f / g_Resolution.y);

    float cd = g_Depth.Sample(g_Sampler, uv);
    if (cd <= 0.0f) return 0.0f;  // 流体なしのピクセル

    float wSum = 0.0f;
    float dSum = 0.0f;
    int R = clamp((int)g_BlurRadius, 1, 5);

    for (int dy = -R; dy <= R; ++dy)
    for (int dx = -R; dx <= R; ++dx) {
        float2 s = uv + float2(dx, dy) * step;
        float  d = g_Depth.Sample(g_Sampler, s);
        if (d <= 0.0f) continue;  // 流体なしサンプルは除外

        // 空間的ガウス重み
        float sw = exp(-float(dx*dx + dy*dy) / (2.0f * g_BlurRadius * g_BlurRadius));
        // バイラテラル深度重み (深度差が大きいほど重みが減る)
        float dd = (d - cd);
        float rw = exp(-dd * dd * g_DepthFalloff);

        float w   = sw * rw;
        dSum += d * w;
        wSum += w;
    }

    return (wSum > 1e-6f) ? dSum / wSum : 0.0f;
}
