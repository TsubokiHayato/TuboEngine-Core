#pragma once
#include "Vector3.h"

/// @brief SPH 粒子データ (CPU & GPU 共通レイアウト)
/// HLSL struct SphParticle と完全一致させること (64 bytes, 16-byte aligned)
struct SphParticle {
    TuboEngine::Math::Vector3 position;  // float3 : 12 bytes
    float density  = 0.0f;               // float  :  4 bytes  → 16
    TuboEngine::Math::Vector3 velocity;  // float3 : 12 bytes
    float pressure = 0.0f;               // float  :  4 bytes  → 32
    TuboEngine::Math::Vector3 force;     // float3 : 12 bytes
    float _pad0    = 0.0f;               // float  :  4 bytes  → 48
    TuboEngine::Math::Vector3 xsph;      // float3 : 12 bytes  XSPH 速度補正
    float _pad1    = 0.0f;               // float  :  4 bytes  → 64
};
