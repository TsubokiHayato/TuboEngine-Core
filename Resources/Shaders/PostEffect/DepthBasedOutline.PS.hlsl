#include "CopyImage.hlsli"
cbuffer gMaterial : register(b0)
{
    float4x4 projectionInverce; // プロジェクション逆行列
    float4 outlineColor;        //　アウトライン色
    float outlineThickness;     //　アウトライン太さ
    float outlineThreshold;     // エッジしきい値
};
Texture2D<float4> gTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// Prewittフィルタ用カーネル（横方向・縦方向）
static const float kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};


static const float2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};


// RGBを輝度に変換する関数
float Luminance(float3 v)
{
    return dot(v, float3(0.2125f, 0.7154f, 0.0721f));
}



PixelShaderOutput main(VertexShaderOutput input)
{
    float2 difference = float2(0.0f, 0.0f);
    uint width, height;
    gTexture.GetDimensions(width, height);

    // アウトライン太さに応じてサンプリング間隔を調整
    float2 uvStepSize = float2(outlineThickness / width, outlineThickness / height);

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            float ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);
            float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), projectionInverce);
            float viewZ = viewSpace.z * rcp(viewSpace.w);

            difference.x += viewZ * kPrewittHorizontalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];
        }
    }

    float weight = length(difference);

    // しきい値でエッジ判定
    float edge = step(outlineThreshold, weight);

    PixelShaderOutput output;
    float3 baseColor = gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.rgb = lerp(baseColor, outlineColor.rgb, edge);
    output.color.a = 1.0f;
    return output;
}