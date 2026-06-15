struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct VHSParams
{
    float time;
    float intensity;
    float scanlineIntensity;
    float chromaticAberration;
};

ConstantBuffer<VHSParams> gParams : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float random(float2 st)
{
    return frac(sin(dot(st.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float2 uv = input.texcoord;
    
    // 微細なジッタ（揺れ）
    float jitter = (random(float2(gParams.time, 0.0)) - 0.5) * 0.002 * gParams.intensity;
    uv.x += jitter;

    // 色収差 (Chromatic Aberration)
    float shift = 0.005 * gParams.chromaticAberration * gParams.intensity;
    float4 colR = gTexture.Sample(gSampler, uv + float2(shift, 0.0));
    float4 colG = gTexture.Sample(gSampler, uv);
    float4 colB = gTexture.Sample(gSampler, uv - float2(shift, 0.0));
    
    float4 color = float4(colR.r, colG.g, colB.b, colG.a);

    // 走査線 (Scanlines)
    float scanline = sin(uv.y * 800.0) * 0.04 * gParams.scanlineIntensity * gParams.intensity;
    color.rgb -= scanline;

    // 粒子ノイズ (Grain)
    float grain = (random(uv + gParams.time) - 0.5) * 0.05 * gParams.intensity;
    color.rgb += grain;

    // ビネット (Vignette)
    float2 d = abs(uv - 0.5) * 2.0;
    float vignette = 1.0 - dot(d, d) * 0.2 * gParams.intensity;
    color.rgb *= vignette;

    output.color = color;
    return output;
}
