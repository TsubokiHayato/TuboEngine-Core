// CsForce.hlsl — 圧力・粘性・重力計算パス (空間ハッシュで近傍探索)
#include "SphCommon.hlsli"

RWStructuredBuffer<SphParticle> g_Particles  : register(u0);
RWStructuredBuffer<int>         g_GridCounts : register(u2);
RWStructuredBuffer<int>         g_GridCells  : register(u3);

[numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint i = tid.x;
    if ((int)i >= g_ParticleCount) return;

    float3 pi  = g_Particles[i].position;
    float  rho = g_Particles[i].density;
    float  pri = g_Particles[i].pressure;
    float3 vi  = g_Particles[i].velocity;

    float3 fPressure  = float3(0, 0, 0);
    float3 fViscosity = float3(0, 0, 0);
    float3 xsphSum    = float3(0, 0, 0);
    float3 colorGrad  = float3(0, 0, 0);  // 表面張力: カラーフィールド勾配 ∇c
    float  colorLap   = 0.0f;             // 表面張力: カラーフィールド ラプラシアン ∇²c

    // ---- 空間ハッシュ: 周囲 27 セルのみ探索 (O(N²)→O(N×近傍数)) ----
    int3 cc = SphCellCoord(pi);
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        int3 nc = cc + int3(dx, dy, dz);
        if (any(nc < int3(0, 0, 0)) || any(nc >= g_GridDim)) continue;
        int cell = SphCellIndex(nc);
        int cnt  = min(g_GridCounts[cell], g_MaxPerCell);
        for (int s = 0; s < cnt; ++s) {
            int j = g_GridCells[cell * g_MaxPerCell + s];
            if (j == (int)i) continue;

            float3 rij = pi - g_Particles[j].position;
            float  r   = length(rij);
            if (r >= g_H || r < 1e-6f) continue;

            float  rhoj = g_Particles[j].density;
            float  prj  = g_Particles[j].pressure;
            float3 vj   = g_Particles[j].velocity;

            // 圧力項
            fPressure  += KernelSpikyGrad(rij, r, g_H) *
                          (-g_Mass * (pri + prj) / (2.0f * rhoj));
            // 粘性項
            fViscosity += (vj - vi) * (g_Viscosity * g_Mass / rhoj * KernelViscLap(r, g_H));
            // XSPH 速度補正: 近傍との速度差を Poly6 で重み付け平均
            xsphSum    += (vj - vi) * (g_Mass / rhoj * KernelPoly6(r * r, g_H));
            // 表面張力: カラーフィールドの勾配・ラプラシアン
            colorGrad  += KernelSpikyGrad(rij, r, g_H) * (g_Mass / rhoj);
            colorLap   += KernelViscLap(r, g_H) * (g_Mass / rhoj);
        }
    }

    // XSPH 補正速度を保存 (Integrate で位置更新に使用)
    g_Particles[i].xsph = g_XsphCoeff * xsphSum;

    float3 fGravity = float3(0.0f, g_Gravity * rho, 0.0f);

    // ---- ミラーパーティクル法 (境界圧力補正) ----
    // 鏡像粒子は静止・同圧力・同密度と仮定し圧力力のみ追加
    // 粘性項は速度0のミラー粒子から計算すると減衰過多になるため省略
    {
        float3 p = pi;
        float3 mirrorPos;
        float3 rij;
        float  r;

        // マクロ的に6面書くと冗長なので関数的に記述
        // X-
        mirrorPos = float3(2.0f*g_BoundMin.x - p.x, p.y, p.z);
        rij = p - mirrorPos; r = length(rij);
        if (r < g_H && r > 1e-6f)
            fPressure += KernelSpikyGrad(rij, r, g_H) * (-g_Mass * (pri + pri) / (2.0f * rho));
        // X+
        mirrorPos = float3(2.0f*g_BoundMax.x - p.x, p.y, p.z);
        rij = p - mirrorPos; r = length(rij);
        if (r < g_H && r > 1e-6f)
            fPressure += KernelSpikyGrad(rij, r, g_H) * (-g_Mass * (pri + pri) / (2.0f * rho));
        // Y- (床)
        mirrorPos = float3(p.x, 2.0f*g_BoundMin.y - p.y, p.z);
        rij = p - mirrorPos; r = length(rij);
        if (r < g_H && r > 1e-6f)
            fPressure += KernelSpikyGrad(rij, r, g_H) * (-g_Mass * (pri + pri) / (2.0f * rho));
        // Y+ (天井)
        mirrorPos = float3(p.x, 2.0f*g_BoundMax.y - p.y, p.z);
        rij = p - mirrorPos; r = length(rij);
        if (r < g_H && r > 1e-6f)
            fPressure += KernelSpikyGrad(rij, r, g_H) * (-g_Mass * (pri + pri) / (2.0f * rho));
        // Z-
        mirrorPos = float3(p.x, p.y, 2.0f*g_BoundMin.z - p.z);
        rij = p - mirrorPos; r = length(rij);
        if (r < g_H && r > 1e-6f)
            fPressure += KernelSpikyGrad(rij, r, g_H) * (-g_Mass * (pri + pri) / (2.0f * rho));
        // Z+
        mirrorPos = float3(p.x, p.y, 2.0f*g_BoundMax.z - p.z);
        rij = p - mirrorPos; r = length(rij);
        if (r < g_H && r > 1e-6f)
            fPressure += KernelSpikyGrad(rij, r, g_H) * (-g_Mass * (pri + pri) / (2.0f * rho));
    }

    // ---- SDF 境界ミラー粒子による圧力補正 ----
    // 障害物: 表面の外側 h/2 以内 (d > 0)
    // コンテナ: 壁の内側 h/2 以内 (d < 0)  ← 符号が逆なだけで同じ式が使える
    for (int si = 0; si < g_SdfCount; ++si) {
        float  d   = SdfEval(g_SdfShapes[si], pi);
        bool   isC = SdfIsContainer(g_SdfShapes[si]);
        bool   apply = isC ? (d < 0.0f && d > -g_H * 0.5f)
                           : (d > 0.0f && d <  g_H * 0.5f);
        if (apply) {
            float3 n     = SdfNormal(g_SdfShapes[si], pi);
            float3 rij_m = n * (2.0f * d);  // d>0: 外向き / d<0: 内向き (自然に反転)
            float  r_m   = abs(2.0f * d);
            fPressure   += KernelSpikyGrad(rij_m, r_m, g_H) *
                           (-g_Mass * (pri + pri) / (2.0f * rho));
        }
    }

    // ---- 外力 (力点から放射状に押す/引く) ----
    float3 fExternal = float3(0, 0, 0);
    if (g_ExtForceActive != 0) {
        float3 toP = pi - g_ExtForcePos;
        float  d   = length(toP);
        if (d < g_ExtForceRadius && d > 1e-4f) {
            float falloff = 1.0f - d / g_ExtForceRadius;  // 中心ほど強い
            fExternal = (toP / d) * (g_ExtForceStrength * falloff * rho);
        }
    }

    // ---- 表面張力 (Müller 2003 カラーフィールド法) ----
    // 表面付近 (|∇c| が大きい) の粒子に、曲率に比例した内向きの力を加える
    float3 fSurface = float3(0, 0, 0);
    float  nLen = length(colorGrad);
    if (nLen > 0.5f) {  // 表面付近のみ (内部は ∇c がほぼ 0)
        fSurface = -g_SurfaceTension * colorLap * (colorGrad / nLen);
    }

    g_Particles[i].force = fPressure + fViscosity + fGravity + fExternal + fSurface;
}
