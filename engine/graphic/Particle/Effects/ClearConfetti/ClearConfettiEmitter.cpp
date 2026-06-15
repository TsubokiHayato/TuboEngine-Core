#include "ClearConfettiEmitter.h"
#include <random>

ParticleInfo ClearConfettiEmitter::GenerateParticle() {
    ParticleInfo p{};
    // 位置: プリセットの範囲内 + エミッタ変換
    std::uniform_real_distribution<float> dx(preset_.posMin.x, preset_.posMax.x);
    std::uniform_real_distribution<float> dy(preset_.posMin.y, preset_.posMax.y);
    std::uniform_real_distribution<float> dz(preset_.posMin.z, preset_.posMax.z);
	p.transform.translate = preset_.emitterTransform.translate + TuboEngine::Math::Vector3{dx(rng_), dy(rng_), dz(rng_)};

    // スケール: 少しばらつかせる（start を基準）
    std::uniform_real_distribution<float> sx(preset_.scaleStart.x * 0.8f, preset_.scaleStart.x * 1.2f);
    std::uniform_real_distribution<float> sy(preset_.scaleStart.y * 0.8f, preset_.scaleStart.y * 1.2f);
    p.transform.scale = { sx(rng_), sy(rng_), preset_.scaleStart.z };

    // 回転ランダム
    std::uniform_real_distribution<float> rz(preset_.initialRotRangeZ.x, preset_.initialRotRangeZ.y);
    p.transform.rotate = { 0.0f, 0.0f, rz(rng_) };

    // 速度: 下方向メイン、横に少し散らす
    std::uniform_real_distribution<float> vx(preset_.velMin.x * 0.5f, preset_.velMax.x * 0.5f);
    std::uniform_real_distribution<float> vy(preset_.velMin.y, preset_.velMax.y);
    std::uniform_real_distribution<float> vz(preset_.velMin.z, preset_.velMax.z);
    p.velocity = { vx(rng_), vy(rng_), vz(rng_) };

    // 色: 緩やかに start->end の範囲でランダム
    std::uniform_real_distribution<float> cr(preset_.colorStart.x, preset_.colorEnd.x);
    std::uniform_real_distribution<float> cg(preset_.colorStart.y, preset_.colorEnd.y);
    std::uniform_real_distribution<float> cb(preset_.colorStart.z, preset_.colorEnd.z);
    std::uniform_real_distribution<float> ca(preset_.colorStart.w, preset_.colorEnd.w);
    p.color = { cr(rng_), cg(rng_), cb(rng_), ca(rng_) };

    // 寿命
    std::uniform_real_distribution<float> lifeDist(preset_.lifeMin, preset_.lifeMax);
    p.lifeTime = lifeDist(rng_);
    p.currentTime = 0.0f;

    return p;
}