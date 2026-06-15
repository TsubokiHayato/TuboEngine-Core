// CsBuildGrid.hlsl — 各粒子を所属セルに登録する
// InterlockedAdd でセル内スロットを確保し、粒子インデックスを書き込む
// Dispatch 数 = ceil(numParticles / 256)
#include "SphCommon.hlsli"

RWStructuredBuffer<SphParticle> g_Particles  : register(u0);
RWStructuredBuffer<int>         g_GridCounts : register(u2);
RWStructuredBuffer<int>         g_GridCells  : register(u3);

[numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint i = tid.x;
    if ((int)i >= g_ParticleCount) return;

    int3 cc   = SphCellCoord(g_Particles[i].position);
    int  cell = SphCellIndex(cc);

    int slot;
    InterlockedAdd(g_GridCounts[cell], 1, slot);

    // 容量を超えた粒子は登録しない (近傍探索でわずかに欠落するが破綻しない)
    if (slot < g_MaxPerCell) {
        g_GridCells[cell * g_MaxPerCell + slot] = (int)i;
    }
}
