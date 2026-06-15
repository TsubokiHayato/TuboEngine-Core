// SphFluidShade.PS.hlsl — ブラー済み深度から法線再構築 + 水面シェーディング
// アルファブレンドでシーン上に合成 (流体なし → discard)

#include "../PostEffect/CopyImage.hlsli"

cbuffer ShadeParams : register(b0) {
    float2 g_Resolution;
    float  g_NormalScale;    // 法線の鋭さ (深度勾配の増幅率)
    float  g_SpecPower;      // Blinn-Phong 鏡面反射の鋭さ
    float4 g_WaterColor;     // 基本水色
    float3 g_LightDir;       // ビュー空間ライト方向 (from camera toward light)
    float  g_FresnelBias;    // フレネル最小値 (0=完全透明, 1=完全不透明)
};

Texture2D<float> g_BlurredDepth : register(t0);
SamplerState     g_Sampler      : register(s0);

float4 main(VertexShaderOutput input) : SV_Target0 {
    float2 uv   = input.texcoord;
    float2 step = float2(1.0f / g_Resolution.x, 1.0f / g_Resolution.y);

    float d = g_BlurredDepth.Sample(g_Sampler, uv);
    if (d <= 0.0f) discard;  // 流体なし → 既存シーンを透過

    // 隣接ピクセルの深度 (0=流体なし → 中心値で代替してエッジを保護)
    float dR = g_BlurredDepth.Sample(g_Sampler, uv + float2( step.x, 0));
    float dL = g_BlurredDepth.Sample(g_Sampler, uv + float2(-step.x, 0));
    float dU = g_BlurredDepth.Sample(g_Sampler, uv + float2(0, -step.y));
    float dD = g_BlurredDepth.Sample(g_Sampler, uv + float2(0,  step.y));
    if (dR <= 0) dR = d; if (dL <= 0) dL = d;
    if (dU <= 0) dU = d; if (dD <= 0) dD = d;

    // 深度勾配からスクリーン空間近似法線
    // dL-dR が正 → 左が浅い → 法線は +X 方向へ傾く
    float3 N = normalize(float3((dL - dR) * g_NormalScale,
                                (dD - dU) * g_NormalScale,
                                1.0f));

    // ライティング (Blinn-Phong)
    float3 L = normalize(-g_LightDir);       // 光源方向 (逆転して入射方向へ)
    float3 V = float3(0.0f, 0.0f, -1.0f);   // カメラ方向 (ビュー空間 -Z)
    float3 H = normalize(L + V);

    float diff = saturate(dot(N, L));
    float spec = pow(saturate(dot(N, H)), g_SpecPower);

    // フレネル (視線と法線の角度で透明度変化)
    float cosA   = saturate(-dot(N, V));     // 正の値にするため符号反転
    float fresnel = g_FresnelBias + (1.0f - g_FresnelBias) * pow(1.0f - cosA, 5.0f);

    // 最終カラー: 拡散 + 鏡面
    float3 col = g_WaterColor.rgb * (0.15f + 0.85f * diff)
               + float3(1.0f, 1.0f, 1.0f) * spec * 0.7f;
    float  a   = saturate(fresnel + 0.45f);

    return float4(col, a);
}
