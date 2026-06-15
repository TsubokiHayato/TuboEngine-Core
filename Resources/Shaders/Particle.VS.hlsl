#include"Particle.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4 Color;
};

StructuredBuffer<TransformationMatrix> gParticle : register(t0);


struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    
};

VertexShaderOutPut main(VertexShaderInput input,uint instanceId : SV_InstanceID)
{
    VertexShaderOutPut output;
    
    output.position = mul(input.position, gParticle[instanceId].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gParticle[instanceId].World));
    output.color = gParticle[instanceId].Color;
    return output;
}
