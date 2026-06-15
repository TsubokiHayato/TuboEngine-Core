#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer FlickerGlowParams : register(b0)
{
    float time;           // 経過時間
    float intensity;      // 全体の強さ
    float noiseAmount;    // ノイズ量
    float glowStrength;   // グローの乗算
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 2D ノイズ
float rand2d(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898f,78.233f))) * 43758.5453f);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 src = gTexture.Sample(gSampler, input.texcoord);

    // 徐々に点灯 (0→1 のイージングを少し早めに立ち上げる)
    float x = saturate(time * intensity);
    float lightFactor = x * x * (3.0f - 2.0f * x); // smoothstep
    lightFactor = pow(lightFactor, 0.6f);

    // チラつきノイズ（カラー寄り＋低周波ゆらぎ）
    float2 noiseUV = input.texcoord * 400.0f + float2(time * 37.0f, time * 17.0f);
    float nBase = rand2d(noiseUV);
    float nR = nBase;
    float nG = rand2d(noiseUV + 10.0f);
    float nB = rand2d(noiseUV + 20.0f);

    float3 colorBias = float3(1.0f, 0.9f, 1.1f); // ほんのりマゼンタ寄り
    float3 noiseRGB = ((float3(nR, nG, nB) - 0.5f) * colorBias) * 2.0f * noiseAmount;

    // 低周波の全体フリッカー
    float flickerLow = 0.5f + 0.5f * sin(time * 5.0f);
    noiseRGB *= lerp(0.5f, 1.5f, flickerLow);

    // ベース色 + ノイズ
    float3 base = src.rgb * lightFactor + noiseRGB;

    // 軽いグロー : 9tap 加重平均＋明るいところだけ強く光らせる
    uint w, h;
    gTexture.GetDimensions(w, h);
    float2 texel = 1.0f / float2(w, h);

    float3 sum = base;
    float weightSum = 1.0f;

    // 上下左右
    float w1 = 0.8f;
    sum += gTexture.Sample(gSampler, input.texcoord + float2( texel.x,  0.0f)).rgb * w1;
    sum += gTexture.Sample(gSampler, input.texcoord + float2(-texel.x,  0.0f)).rgb * w1;
    sum += gTexture.Sample(gSampler, input.texcoord + float2( 0.0f,  texel.y)).rgb * w1;
    sum += gTexture.Sample(gSampler, input.texcoord + float2( 0.0f, -texel.y)).rgb * w1;
    weightSum += w1 * 4.0f;

    // 斜め
    float w2 = 0.5f;
    sum += gTexture.Sample(gSampler, input.texcoord + texel * float2( 1,  1)).rgb * w2;
    sum += gTexture.Sample(gSampler, input.texcoord + texel * float2(-1,  1)).rgb * w2;
    sum += gTexture.Sample(gSampler, input.texcoord + texel * float2( 1, -1)).rgb * w2;
    sum += gTexture.Sample(gSampler, input.texcoord + texel * float2(-1, -1)).rgb * w2;
    weightSum += w2 * 4.0f;

    float3 glow = sum / weightSum;

    // 明るい部分だけグローを強めるマスク
    float luminance = dot(src.rgb, float3(0.2125f, 0.7154f, 0.0721f));
    float glowMask  = saturate((luminance - 0.4f) * 3.0f); // 閾値と強さは好みで調整

    float3 finalColor = lerp(base, glow, glowStrength * glowMask);

    output.color = float4(finalColor, src.a);
    return output;
}
