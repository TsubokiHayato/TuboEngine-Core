#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


cbuffer RadialBlurParams : register(b0)
{
    float2 center; // ブラー中心
    float blurWidth; // ブラー強さ
    float pad; // 16バイトアライメント
}


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // ブラーの中心（画面中央）
   
    // サンプリング数
    const int kNumSamples = 10;

    // 中心から現在のuvへの方向ベクトル
    float2 direction = input.texcoord - center;

    float3 sum = float3(0.0f, 0.0f, 0.0f);

    // 放射状にサンプリング
    for (int i = 0; i < kNumSamples; ++i)
    {
        float t = float(i) / (kNumSamples - 1);
        float2 sampleUV = input.texcoord - direction * blurWidth * t;
        sum += gTexture.Sample(gSampler, sampleUV).rgb;
    }

    sum /= kNumSamples;
    output.color = float4(sum, 1.0f);
    return output;
}
