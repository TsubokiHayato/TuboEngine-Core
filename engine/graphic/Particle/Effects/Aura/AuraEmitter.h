#pragma once
#include "Effects/Primitive/PrimitiveEmitter.h"

// プレイヤー周りの常時オーラ用エミッター
class AuraEmitter : public PrimitiveEmitter {
public:
    // 外部からデフォルトプリセットで作るときに用いる初期化
    void Initialize(const ParticlePreset& preset) override;

protected:
    // 粒子固有の微調整（わずかに外向きの速度等）
    ParticleInfo GenerateParticle() override;
};