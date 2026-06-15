#include "CopyImage.hlsli"

cbuffer ToonParams : register(b0)
{
    int stepCount;
    float toonRate;
    float3 shadowColor;
    float3 highlightColor;
    float2 padding;
}
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャカラー取得
    float4 color = gTexture.Sample(gSampler, input.texcoord);

    // 明度（輝度）を計算
    float brightness = dot(color.rgb, float3(0.2125f, 0.7154f, 0.0721f));

    // 段階化
    float stepBrightness = floor(brightness * stepCount) / stepCount;
    // 段階ごとに色を補間
    float3 toonColor = lerp(shadowColor, highlightColor, stepBrightness);

    // 色を量子化
    float3 quantizedColor = floor(color.rgb * stepCount) / stepCount;
    
    float3 blendedColor = lerp(quantizedColor, toonColor, toonRate);

    output.color = float4(blendedColor, color.a);
    return output;
}