#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// 3つの異なる乱数でRGBノイズ
float rand2d1d(float2 uv, float seed)
{
    return frac(sin(dot(uv, float2(12.9898f + seed, 78.233f - seed))) * 43758.5453f);
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float r = rand2d1d(input.texcoord, 1.0f);
    float g = rand2d1d(input.texcoord, 2.0f);
    float b = rand2d1d(input.texcoord, 3.0f);

    output.color = float4(r, g, b, 1.0f);

    return output;
}
