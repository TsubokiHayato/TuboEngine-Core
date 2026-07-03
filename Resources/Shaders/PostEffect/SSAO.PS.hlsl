#include "CopyImage.hlsli"

// SSAO（スクリーンスペース・アンビエントオクルージョン）
// 深度バッファだけを使う近似実装。各ピクセルの周囲を円状にサンプリングし、
// 「手前に別の面がどれだけあるか（＝くぼみ具合）」で遮蔽を求めて陰影を落とす。
cbuffer gParams : register(b0)
{
    float4x4 projectionInverse; // NDC深度 → ビュー空間へ戻す逆射影行列
    float radius;      // サンプル半径（スクリーンピクセル）
    float bias;        // 自己遮蔽防止のしきい値（ビュー空間距離）
    float intensity;   // AO の強さ
    float power;       // コントラスト（pow でメリハリ）
    float depthRange;  // これ以上離れた深度差は別物体とみなし無視（境界のにじみ防止）
};

Texture2D<float4> gTexture : register(t0);     // シーンカラー
Texture2D<float> gDepthTexture : register(t1); // 深度
SamplerState gSampler : register(s0);          // 線形
SamplerState gSamplerPoint : register(s1);     // ポイント（クランプ）

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// NDC深度 → カメラからの距離（|viewZ|）。x,y に依存しないので中心軸で復元してよい。
float ViewDistFromDepth(float ndcDepth)
{
    float4 v = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), projectionInverse);
    return abs(v.z / v.w);
}

// 円周8方向（回転させて使う基本サンプル方向）
static const float2 kDisk[8] =
{
    float2(1.0f, 0.0f), float2(-1.0f, 0.0f),
    float2(0.0f, 1.0f), float2(0.0f, -1.0f),
    float2(0.70710678f, 0.70710678f), float2(-0.70710678f, 0.70710678f),
    float2(0.70710678f, -0.70710678f), float2(-0.70710678f, -0.70710678f),
};

float Hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 texel = float2(1.0f / width, 1.0f / height);

    float centerDepth = gDepthTexture.Sample(gSamplerPoint, input.texcoord);

    PixelShaderOutput output;
    float3 baseColor = gTexture.Sample(gSampler, input.texcoord).rgb;

    // 背景（遠クリップ）は遮蔽計算しない
    if (centerDepth >= 0.9999f)
    {
        output.color = float4(baseColor, 1.0f);
        return output;
    }

    float centerDist = ViewDistFromDepth(centerDepth);

    // 画素ごとにサンプル方向を回転させてバンディングを軽減
    float ang = Hash(input.texcoord) * 6.2831853f;
    float ca = cos(ang);
    float sa = sin(ang);
    float2x2 rot = float2x2(ca, -sa, sa, ca);

    float occlusion = 0.0f;
    const int kRings = 2;
    int sampleCount = 0;

    for (int r = 1; r <= kRings; ++r)
    {
        float ringScale = (float) r / (float) kRings;
        for (int s = 0; s < 8; ++s)
        {
            float2 dir = mul(kDisk[s], rot);
            float2 uv = input.texcoord + dir * ringScale * radius * texel;

            float sampleDepth = gDepthTexture.Sample(gSamplerPoint, uv);
            float sampleDist = ViewDistFromDepth(sampleDepth);

            // 正 = サンプル面がカメラに近い（手前に何かある＝遮蔽している）
            float diff = centerDist - sampleDist;

            // 深度差が大きすぎる（別物体）ものは無視して境界のにじみを防ぐ
            float rangeCheck = 1.0f - smoothstep(depthRange * 0.5f, depthRange, abs(diff));
            occlusion += (diff > bias ? 1.0f : 0.0f) * rangeCheck;
            sampleCount++;
        }
    }

    occlusion /= (float) sampleCount;
    float ao = saturate(1.0f - occlusion * intensity);
    ao = pow(ao, power);

    output.color = float4(baseColor * ao, 1.0f);
    return output;
}
