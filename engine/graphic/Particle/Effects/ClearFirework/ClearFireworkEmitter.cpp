#include "ClearFireworkEmitter.h"
#include <random>
#include <cmath>

ParticleInfo ClearFireworkEmitter::GenerateParticle() {
    ParticleInfo p{};
    // 中心位置をランダムに選ぶ（プリセットの posMin/posMax をバースト中心範囲とする）
    std::uniform_real_distribution<float> cx(preset_.posMin.x, preset_.posMax.x);
    std::uniform_real_distribution<float> cy(preset_.posMin.y, preset_.posMax.y);
    std::uniform_real_distribution<float> cz(preset_.posMin.z, preset_.posMax.z);
	TuboEngine::Math::Vector3 center = preset_.emitterTransform.translate + TuboEngine::Math::Vector3{cx(rng_), cy(rng_), cz(rng_)};
    p.transform.translate = center;

    // スケール小さめ
    std::uniform_real_distribution<float> sdist(preset_.scaleStart.x * 0.6f, preset_.scaleStart.x * 1.1f);
    float s = sdist(rng_);
    p.transform.scale = { s, s, preset_.scaleStart.z };

    // 放射方向の速度（角度ランダム + スピードレンジを利用）
    std::uniform_real_distribution<float> ang(0.0f, 2.0f * 3.14159265359f);
    // 速度レンジは preset_.velMin.x/velMax.x を速度のレンジとして利用する想定
    float minSpeed = std::min(preset_.velMin.x, preset_.velMax.x);
    float maxSpeed = std::max(preset_.velMin.x, preset_.velMax.x);
    std::uniform_real_distribution<float> speedDist(minSpeed, maxSpeed);
    float speed = speedDist(rng_);
    float a = ang(rng_);
    p.velocity = { std::cos(a) * speed, std::sin(a) * speed, 0.0f };

    // 色
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