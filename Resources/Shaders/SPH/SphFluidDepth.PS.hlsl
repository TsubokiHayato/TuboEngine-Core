// SphFluidDepth.PS.hlsl — 球面インポスターの view-space Z を R32_FLOAT に書き込む
// quadUV が単位円の外 → discard (球シルエット外)

struct VSOut {
    float4 pos      : SV_Position;
    float3 centerVS : TEXCOORD0;
    float2 quadUV   : TEXCOORD1;
    float  radius   : TEXCOORD2;
};

float main(VSOut input) : SV_Target0 {
    float r2 = dot(input.quadUV, input.quadUV);
    if (r2 > 1.0f) discard;  // 球シルエット外

    // 球面フロントの view-space Z を計算
    // 左手系 +Z = 前方。前面は中心より Z が小さい (カメラに近い)
    float dz  = sqrt(max(0.0f, 1.0f - r2)) * input.radius;
    float vsZ = input.centerVS.z - dz;

    return vsZ;  // R32_FLOAT に書き込む
}
