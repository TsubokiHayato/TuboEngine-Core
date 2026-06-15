#pragma once
#include "IParticleEmitter.h"
#include <numbers>
#include <cmath>

class OriginalEmitter : public IParticleEmitter {
protected:
	void BuildGeometry(std::vector<TuboEngine::VertexData>& out) override {
        // 中央クアッド
        out.push_back({ { 1, 1, 0, 1 }, {0,0}, {0,0,1} });
        out.push_back({ {-1, 1, 0, 1 }, {1,0}, {0,0,1} });
        out.push_back({ { 1,-1, 0, 1 }, {0,1}, {0,0,1} });
        out.push_back({ { 1,-1, 0, 1 }, {0,1}, {0,0,1} });
        out.push_back({ {-1, 1, 0, 1 }, {1,0}, {0,0,1} });
        out.push_back({ {-1,-1, 0, 1 }, {1,1}, {0,0,1} });

        // 放射ライン（細長三角形）を円周方向に
        const uint32_t rays = 12;
        const float outer = 1.6f;
        const float inner = 0.2f;
        const float d = 2.0f * std::numbers::pi_v<float> / float(rays);
        for (uint32_t i = 0; i < rays; ++i) {
            float a = i * d;
            float a2 = a + 0.12f; // 幅
			TuboEngine::Math::Vector3 p0{0, 0, 0};
			TuboEngine::Math::Vector3 p1{std::cos(a) * outer, std::sin(a) * outer, 0.0f};
			TuboEngine::Math::Vector3 p2{std::cos(a2) * outer, std::sin(a2) * outer, 0.0f};
            // 三角面
            out.push_back({ {p0.x, p0.y, 0,1}, {0.5f,0.5f}, {0,0,1} });
            out.push_back({ {p1.x, p1.y, 0,1}, {0.0f,0.0f}, {0,0,1} });
            out.push_back({ {p2.x, p2.y, 0,1}, {1.0f,0.0f}, {0,0,1} });
        }
    }

    ParticleInfo GenerateParticle() override {
        ParticleInfo p{};
        // 球状に拡散
        std::uniform_real_distribution<float> life(preset_.lifeMin, preset_.lifeMax);
        std::uniform_real_distribution<float> rr(0.0f, 1.0f);
        std::uniform_real_distribution<float> ang1(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> ang2(0.0f, std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> colR(preset_.colMin.x, preset_.colMax.x);
        std::uniform_real_distribution<float> colG(preset_.colMin.y, preset_.colMax.y);
        std::uniform_real_distribution<float> colB(preset_.colMin.z, preset_.colMax.z);
        std::uniform_real_distribution<float> colA(preset_.colMin.w, preset_.colMax.w);
        std::uniform_real_distribution<float> sx(preset_.scaleMin.x, preset_.scaleMax.x);

        float r = rr(rng_);
        float a = ang1(rng_);
        float b = ang2(rng_);
        // 球面方向ベクトル
		TuboEngine::Math::Vector3 dir{
            r * std::sin(b) * std::cos(a),
            r * std::cos(b),
            r * std::sin(b) * std::sin(a)
        };

        p.transform.translate = {
            std::uniform_real_distribution<float>(preset_.posMin.x, preset_.posMax.x)(rng_),
            std::uniform_real_distribution<float>(preset_.posMin.y, preset_.posMax.y)(rng_),
            std::uniform_real_distribution<float>(preset_.posMin.z, preset_.posMax.z)(rng_)
        };
        p.transform.scale = { sx(rng_), sx(rng_), sx(rng_) };
        p.transform.rotate = { 0,0,0 };

        // 速度: dir 正規化方向に速度範囲（velMin/velMax の x を速度下限、y を上限とみなすなど単純化）
        float speedMin = preset_.velMin.x;
        float speedMax = preset_.velMax.x > speedMin ? preset_.velMax.x : speedMin + 0.01f;
        std::uniform_real_distribution<float> speedDist(speedMin, speedMax);
        float sp = speedDist(rng_);
        p.velocity = { dir.x * sp, dir.y * sp, dir.z * sp };

        p.color = { colR(rng_), colG(rng_), colB(rng_), colA(rng_) };
        p.lifeTime = life(rng_);
        p.currentTime = 0.0f;
        return p;
    }
};