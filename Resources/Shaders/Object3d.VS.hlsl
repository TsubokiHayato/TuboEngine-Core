#include"Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
};

struct InstanceData
{
    float4x4 WVP;
    float4x4 World;
    float4 Color;
};

struct CommonData
{
    int useInstancing;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
ConstantBuffer<CommonData> gCommonData : register(b1);
StructuredBuffer<InstanceData> gInstanceData : register(t0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutPut main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutPut output;
    
    float4x4 wvp;
    float4x4 world;
    float4 color;

    if (gCommonData.useInstancing != 0)
    {
        wvp = gInstanceData[instanceId].WVP;
        world = gInstanceData[instanceId].World;
        color = gInstanceData[instanceId].Color;
    }
    else
    {
        wvp = gTransformationMatrix.WVP;
        world = gTransformationMatrix.World;
        color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    output.position = mul(input.position, wvp);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) world));
    output.worldPosition = mul(input.position, world).xyz;
    output.color = color;

    return output;
}
