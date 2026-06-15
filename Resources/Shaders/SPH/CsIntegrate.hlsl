// CsIntegrate.hlsl — 速度・位置の積分 + AABB 境界反射
#include "SphCommon.hlsli"

RWStructuredBuffer<SphParticle> g_Particles : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint i = tid.x;
    if ((int)i >= g_ParticleCount) return;

    float3 pos  = g_Particles[i].position;
    float3 vel  = g_Particles[i].velocity;
    float3 f    = g_Particles[i].force;
    float  rho  = g_Particles[i].density;
    float3 xsph = g_Particles[i].xsph;

    // セミインプリシット Euler + XSPH 位置補正
    // 速度本体は補正せず、位置更新にのみ近傍平均速度を加える (Monaghan XSPH)
    // → 粒子が個別にバラけず、連続した流体のように動く
    float3 accel = f / rho;
    vel += accel * g_Dt;

    // 速度クランプ (CFL 的な上限): 1 ステップで影響半径の半分以上動かさない
    // → 高速時の数値爆発を防ぐ。g_Dt=0 (一時停止) のときはクランプしない
    if (g_Dt > 1e-8f) {
        float vmax  = 0.5f * g_H / g_Dt;
        float speed = length(vel);
        if (speed > vmax) vel *= vmax / speed;
    }

    pos += (vel + xsph) * g_Dt;

    // AABB 境界反射
    if (pos.x < g_BoundMin.x) { pos.x = g_BoundMin.x; vel.x =  abs(vel.x) * g_Restitution; }
    if (pos.x > g_BoundMax.x) { pos.x = g_BoundMax.x; vel.x = -abs(vel.x) * g_Restitution; }
    if (pos.y < g_BoundMin.y) { pos.y = g_BoundMin.y; vel.y =  abs(vel.y) * g_Restitution; }
    if (pos.y > g_BoundMax.y) { pos.y = g_BoundMax.y; vel.y = -abs(vel.y) * g_Restitution; }
    if (pos.z < g_BoundMin.z) { pos.z = g_BoundMin.z; vel.z =  abs(vel.z) * g_Restitution; }
    if (pos.z > g_BoundMax.z) { pos.z = g_BoundMax.z; vel.z = -abs(vel.z) * g_Restitution; }

    // ---- SDF 境界 ----
    for (int si = 0; si < g_SdfCount; ++si) {
        float  d = SdfEval(g_SdfShapes[si], pos);
        float3 n = SdfNormal(g_SdfShapes[si], pos);

        if (!SdfIsContainer(g_SdfShapes[si])) {
            // 障害物: 内部に侵入したら外へ押し出す
            if (d < 0.0f) {
                pos -= d * n;                       // d<0 なので -d>0 → 外向き
                float vn = dot(vel, n);
                if (vn < 0.0f)
                    vel -= (1.0f + g_Restitution) * vn * n;
            }
        } else {
            // コンテナ: 外部に出たら内へ押し戻す
            if (d > 0.0f) {
                pos -= d * n;                       // d>0, n=外向き → 内向きに戻す
                float vn = dot(vel, n);
                if (vn > 0.0f)                      // 外へ向かっている速度を反射
                    vel -= (1.0f + g_Restitution) * vn * n;
            }
        }
    }

    g_Particles[i].position = pos;
    g_Particles[i].velocity = vel;
}
