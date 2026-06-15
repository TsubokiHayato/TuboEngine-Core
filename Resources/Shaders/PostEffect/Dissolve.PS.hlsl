#include "CopyImage.hlsli"
cbuffer DissolveParams : register(b0)
{
    float dissolveThreshold; // ディゾルブの閾値
    float3 edgeColor; //エッジの色
    float edgeStrength; //エッジの強さ
    float edgeWidth; //エッジの幅
}
Texture2D<float4> gTexture : register(t0);
Texture2D<float> gMaskTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float mask = gMaskTexture.Sample(gSampler, input.texcoord);
    
    //maskが閾値以下ならdiscard
    if (mask <= dissolveThreshold)
    {
        discard;
    }
    float edge = 1.0f - smoothstep(0.5f, 0.5f + edgeWidth, mask);
    output.color = gTexture.Sample(gSampler, input.texcoord);
    output.color.rgb += edge * edgeColor * edgeStrength;
    output.color.a *= mask; // アルファ値をマスクで制御
    
    return output;
}
