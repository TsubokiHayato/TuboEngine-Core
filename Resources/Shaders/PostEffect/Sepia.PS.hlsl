#include "CopyImage.hlsli"

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

    float value = dot(color.rgb, float3(0.21125f, 0.7154f, 0.0721f));
    
    output.color.rgb = value * float3(1.0f, 74.0f / 107.0f, 43.0f / 107.0f);
    output.color.a = color.a;
    return output;
}

