#include "CopyImage.hlsli"

cbuffer VignetteParams : register(b0)
{
    float vignetteScale; // 例: 16.0f
    float vignettePower; // 例: 0.8f
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
    float4 color = gTexture.Sample(gSampler, input.texcoord);
    float2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    float vignette = correct.x * correct.y * vignetteScale;
    vignette = saturate(pow(vignette, vignettePower));
    output.color = float4(color.rgb * vignette, color.a);
    return output;
}
