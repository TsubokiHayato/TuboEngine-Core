#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// gMaterial 構造体に time が含まれている前提
cbuffer MaterialBuffer : register(b0)
{
    float time; 
   
}

float rand2d1d(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


PixelShaderOutput main(VertexShaderOutput input)
{
    
    PixelShaderOutput output;

    // 時間を掛けてノイズを変化させる
    float noise = rand2d1d(input.texcoord * time);

    output.color = float4(noise, noise, noise, 1.0f);

    return output;
}