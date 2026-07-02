#pragma once
#include "../Primitive/PrimitiveEmitter.h"

/// <summary>
/// クリア演出用の紙吹雪パーティクルエミッター。
/// </summary>
class ClearConfettiEmitter : public PrimitiveEmitter {
public:
    /// <summary>
    /// 初期化処理。
    /// </summary>
    void Initialize(const ParticlePreset& preset) override {
        PrimitiveEmitter::Initialize(preset);
    }

protected:
    /// <summary>
    /// パーティクル1個分の初期状態を生成する。
    /// </summary>
    ParticleInfo GenerateParticle() override;
};