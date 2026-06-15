// CsClearGrid.hlsl — 空間ハッシュのセルカウンタを 0 クリア
// Dispatch 数 = ceil(numCells / 256)
#include "SphCommon.hlsli"

RWStructuredBuffer<int> g_GridCounts : register(u2);

[numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    int numCells = g_GridDim.x * g_GridDim.y * g_GridDim.z;
    if ((int)tid.x >= numCells) return;
    g_GridCounts[tid.x] = 0;
}
