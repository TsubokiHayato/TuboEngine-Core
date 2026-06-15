#include "CopyImage.hlsli"

cbuffer GaussianParams : register(b0)
{
    float sigma;
    float pad[3]; // 16-byte alignment
}

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float PI = 3.14159265358979323846f;

float gauss(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) / (2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) / denominator;
}

// 5x5カーネルのガウシアンブラー

//3x3では効果が薄いので5x5に変更
//重いので注意
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(1.0f / width, 1.0f / height);

    float weightSum = 0.0f;
    float3 sum = float3(0.0f, 0.0f, 0.0f);

    // 5x5カーネル: -2～+2
    [unroll]
    for (int x = -2; x <= 2; ++x)
    {
        [unroll]
        for (int y = -2; y <= 2; ++y)
        {
            float2 offset = float2(x, y);
            float2 texcoord = input.texcoord + offset * uvStepSize;
            float weight = gauss(offset.x, offset.y, sigma);
            float3 color = gTexture.Sample(gSampler, texcoord).rgb;
            sum += color * weight;
            weightSum += weight;
        }
    }

    sum /= weightSum;
    output.color = float4(sum, 1.0f);
    return output;
}
