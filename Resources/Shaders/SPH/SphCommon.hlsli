// SphCommon.hlsli — SPH 全シェーダー共通定義
// NOTE: SPH_MAX_SDF_SHAPES は C++ 側の kMaxSdfShapes (SphComputePipeline.h) と必ず一致させること
#define SPH_MAX_SDF_SHAPES 16

static const float SPH_PI = 3.14159265359f;

// ---- GPU 上の粒子データ (SphParticle.h と完全一致, 64 bytes) ----
struct SphParticle {
    float3 position;
    float  density;
    float3 velocity;
    float  pressure;
    float3 force;
    float  _pad0;
    float3 xsph;       // XSPH 速度補正
    float  _pad1;
};

// ---- GPU 上のインスタンスデータ (SphGPUInstance と一致) ----
struct SphInstance {
    float4x4 WVP;
    float4x4 World;
    float4   color;
};

// ---- SDF 障害物形状 (SdfShapeGpu と完全一致, 32 bytes) ----
struct SdfShape {
    float3 center;      // 0-11  中心座標
    int    type;        // 12-15  0=sphere, 1=box
    float3 halfExtents; // 16-27  sphere: x=radius; box: 各軸の半辺長
    float  _pad;        // 28-31
};

// ---- シミュレーションパラメーター ----
cbuffer SphParams : register(b0) {
    int    g_ParticleCount;    // 粒子数
    float  g_H;                // smoothingRadius
    float  g_RestDensity;      // restDensity
    float  g_Stiffness;        // stiffness
    float  g_Viscosity;        // viscosity
    float  g_Mass;             // particleMass
    float  g_Gravity;          // gravity (Y)
    float  g_Restitution;      // restitution
    float3 g_BoundMin;         // boundMin
    float  g_Dt;               // dt per substep
    float3 g_BoundMax;         // boundMax
    float  g_SpeedMax;         // 色補間最大速度
    float4 g_ColorLow;         // colorLow
    float4 g_ColorHigh;        // colorHigh
    float  g_ParticleRadius;   // 粒子描画半径
    float  g_XsphCoeff;        // XSPH 速度補正係数 ε
    float  _cbPad1, _cbPad2;
    float4x4 g_ViewProj;       // view-projection matrix (16-byte aligned)
    // ---- 空間ハッシュ (近傍探索高速化) ----
    int3   g_GridDim;          // グリッド各軸セル数
    float  g_CellSize;         // セルサイズ (= 初期 smoothingRadius)
    float3 g_GridMin;          // グリッド原点 (= 初期 boundMin)
    int    g_MaxPerCell;       // セルあたり最大粒子数
    // ---- 外力 (力点から放射状) ----
    float3 g_ExtForcePos;      // 力点中心
    float  g_ExtForceRadius;   // 影響半径
    float  g_ExtForceStrength; // 強さ (正=押し出し, 負=引き寄せ)
    int    g_ExtForceActive;   // 有効フラグ
    float  g_SurfaceTension;   // 表面張力係数 σ
    int    g_SdfCount;         // SDF 障害物数 (0..SPH_MAX_SDF_SHAPES)
    // g_SdfCount の終端 = byte 240 = 16-byte 境界のため追加パディング不要
    SdfShape g_SdfShapes[SPH_MAX_SDF_SHAPES]; // 240 + 16×32=512 bytes → 合計 752 bytes
};

// ---- SPH カーネル ----
float KernelPoly6(float r2, float h) {
    if (r2 >= h * h) return 0.0f;
    float d = h * h - r2;
    return (315.0f / (64.0f * SPH_PI * pow(h, 9.0f))) * d * d * d;
}

float3 KernelSpikyGrad(float3 rij, float r, float h) {
    if (r >= h || r < 1e-6f) return float3(0, 0, 0);
    float d = h - r;
    return rij * (-(45.0f / (SPH_PI * pow(h, 6.0f))) * d * d / r);
}

float KernelViscLap(float r, float h) {
    if (r >= h) return 0.0f;
    return (45.0f / (SPH_PI * pow(h, 6.0f))) * (h - r);
}

// ---- 空間ハッシュ ヘルパー ----
// 粒子位置からグリッドセル座標を求める (範囲外は端セルにクランプ)
int3 SphCellCoord(float3 pos) {
    int3 c = (int3)floor((pos - g_GridMin) / g_CellSize);
    return clamp(c, int3(0, 0, 0), g_GridDim - int3(1, 1, 1));
}

// セル座標を 1 次元インデックスに変換
int SphCellIndex(int3 c) {
    return c.x + c.y * g_GridDim.x + c.z * g_GridDim.x * g_GridDim.y;
}

// ---- SDF ヘルパー ----
//
//  type 値の意味:
//    0 = 球 障害物      (流体を外へ押し出す)
//    1 = 箱 障害物
//    2 = 球 コンテナ    (流体を内へ押し戻す)
//    3 = 箱 コンテナ
//    4 = 円柱 コンテナ  (halfExtents.x=半径, halfExtents.y=半高さ)

bool SdfIsContainer(SdfShape s) { return s.type >= 2; }

// 形状の基底種別 (0=球, 1=箱, 2=円柱)
int SdfBaseType(SdfShape s) {
    if (s.type == 0 || s.type == 2) return 0;
    if (s.type == 1 || s.type == 3) return 1;
    return 2;  // 4 = 円柱コンテナ
}

// 球 SDF
float SdfSphere(float3 p, float3 center, float radius) {
    return length(p - center) - radius;
}

// 箱 SDF
float SdfBox(float3 p, float3 center, float3 half) {
    float3 q = abs(p - center) - half;
    return length(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
}

// 円柱 SDF (Y 軸方向, halfExtents.x=半径, halfExtents.y=半高さ)
float SdfCylinder(float3 p, float3 center, float radius, float halfH) {
    float3 d = p - center;
    float2 q = float2(length(d.xz) - radius, abs(d.y) - halfH);
    return min(max(q.x, q.y), 0.0f) + length(max(q, 0.0f));
}

// 形状に応じた SDF 評価
float SdfEval(SdfShape s, float3 p) {
    int bt = SdfBaseType(s);
    if (bt == 0) return SdfSphere(p, s.center, s.halfExtents.x);
    if (bt == 1) return SdfBox(p, s.center, s.halfExtents);
    return SdfCylinder(p, s.center, s.halfExtents.x, s.halfExtents.y);
}

// 球の外向き法線
float3 SdfSphereNormal(float3 p, float3 center) {
    float3 d = p - center;
    float  l = length(d);
    return l > 1e-6f ? d / l : float3(0, 1, 0);
}

// 箱の外向き法線
float3 SdfBoxNormal(float3 p, float3 center, float3 half) {
    float3 d  = p - center;
    float3 sn = float3(d.x >= 0 ? 1.0f : -1.0f,
                       d.y >= 0 ? 1.0f : -1.0f,
                       d.z >= 0 ? 1.0f : -1.0f);
    float3 q  = abs(d) - half;
    if (all(q <= 0.0f)) {
        if (q.x >= q.y && q.x >= q.z) return float3(sn.x, 0, 0);
        if (q.y >= q.x && q.y >= q.z) return float3(0, sn.y, 0);
        return float3(0, 0, sn.z);
    }
    float3 n = max(q, 0.0f) * sn;
    float  nl = length(n);
    return nl > 1e-6f ? n / nl : float3(0, 1, 0);
}

// 円柱の外向き法線 (Y 軸方向)
float3 SdfCylinderNormal(float3 p, float3 center, float radius, float halfH) {
    float3 d  = p - center;
    float  r  = length(d.xz);
    float2 q  = float2(r - radius, abs(d.y) - halfH);

    if (all(q < 0.0f)) {
        // 内側: 最近傍面の法線
        if (q.x > q.y) {
            return r > 1e-6f ? float3(d.x / r, 0, d.z / r) : float3(1, 0, 0);
        }
        return float3(0, d.y >= 0 ? 1.0f : -1.0f, 0);
    }
    // 外側: SDF 勾配
    float2 qp = max(q, 0.0f);
    float3 n  = float3(0, 0, 0);
    if (qp.x > 0 && r > 1e-6f) n += float3(d.x / r, 0, d.z / r) * qp.x;
    if (qp.y > 0) n += float3(0, d.y >= 0 ? 1.0f : -1.0f, 0) * qp.y;
    float nl = length(n);
    return nl > 1e-6f ? n / nl : float3(0, 1, 0);
}

// 形状に応じた外向き法線
float3 SdfNormal(SdfShape s, float3 p) {
    int bt = SdfBaseType(s);
    if (bt == 0) return SdfSphereNormal(p, s.center);
    if (bt == 1) return SdfBoxNormal(p, s.center, s.halfExtents);
    return SdfCylinderNormal(p, s.center, s.halfExtents.x, s.halfExtents.y);
}
