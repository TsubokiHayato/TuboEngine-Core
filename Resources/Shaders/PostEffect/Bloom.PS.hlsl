#include"CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BloomParams : register(b0)
{
    float threshold; // 明るさの閾値
    float intensity; // Bloomの強さ
    float2 padding;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // 入力テクスチャから色取得
    float4 color = gTexture.Sample(gSampler, input.texcoord);

    // 明るい部分のみ抽出
    float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114));
    float bloomMask = saturate((luminance - threshold) / (1.0 - threshold));

    // Bloom部分（明るい部分のみ）
    float3 bloomColor = color.rgb * bloomMask * intensity;

    // 元の色とBloom色を合成
    float3 finalColor = color.rgb + bloomColor;

    output.color = float4(finalColor, color.a);
    return output;
}