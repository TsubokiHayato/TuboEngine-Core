#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};

static const float kKernel3x3[3][3] =
{
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
};

/*3x3カーネルの平滑化フィルタ*/
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

    float3 sum = float3(0.0f, 0.0f, 0.0f);

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 offset = kIndex3x3[x][y] * uvStepSize;
            float2 texcoord = input.texcoord + offset;
            float3 color = gTexture.Sample(gSampler, texcoord).rgb;
            sum += color * kKernel3x3[x][y];
        }
    }

    output.color = float4(sum, 1.0f);
    return output;
}



/*5x5カーネルの平滑化フィルタ*/
/*
static const int KERNEL_RADIUS = 2; // 5x5カーネル

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

    float3 sum = float3(0.0f, 0.0f, 0.0f);

    // 5x5カーネル
    [unroll]
    for (int x = -KERNEL_RADIUS; x <= KERNEL_RADIUS; ++x)
    {
        [unroll]
        for (int y = -KERNEL_RADIUS; y <= KERNEL_RADIUS; ++y)
        {
            float2 offset = float2(x, y) * uvStepSize;
            float2 texcoord = input.texcoord + offset;
            float3 color = gTexture.Sample(gSampler, texcoord).rgb;
            sum += color;
        }
    }

    sum /= 25.0f; // 5x5=25
    output.color = float4(sum, 1.0f);
    return output;
}
*/