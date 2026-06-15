#pragma once
#include "IParticleEmitter.h"
#include <cmath>
#include <numbers>

class CylinderEmitter : public IParticleEmitter {
protected:
	void BuildGeometry(std::vector<TuboEngine::VertexData>& out) override {
		// 円柱側面のみ（レガシーの簡易版）
		const uint32_t kDiv = 32;
		const float topR = 1.0f;
		const float bottomR = 1.0f;
		const float height = 3.0f;
		const float d = 2.0f * std::numbers::pi_v<float> / float(kDiv);
		for (uint32_t i = 0; i < kDiv; ++i) {
			float a0 = i * d;
			float a1 = (i + 1) * d;
			float s0 = std::sin(a0), c0 = std::cos(a0);
			float s1 = std::sin(a1), c1 = std::cos(a1);
			float u0 = float(i) / float(kDiv);
			float u1 = float(i + 1) / float(kDiv);

			TuboEngine::Math::Vector2 tTop0{u0, 0.0f}, tTop1{u1, 0.0f};
			TuboEngine::Math::Vector2 tBot0{u0, 1.0f}, tBot1{u1, 1.0f};
			// v反転（必要なら）
			tTop0.y = 1.0f - tTop0.y;
			tTop1.y = 1.0f - tTop1.y;
			tBot0.y = 1.0f - tBot0.y;
			tBot1.y = 1.0f - tBot1.y;

			TuboEngine::Math::Vector3 n0{-s0, 0.0f, c0};
			TuboEngine::Math::Vector3 n1{-s1, 0.0f, c1};

			// 2三角 + 追加の分割で少し厚み
			out.push_back({
			    {-s0 * topR, height, c0 * topR, 1.0f},
                tTop0, n0
            });
			out.push_back({
			    {-s1 * topR, height, c1 * topR, 1.0f},
                tTop1, n1
            });
			out.push_back({
			    {-s1 * bottomR, 0.0f, c1 * bottomR, 1.0f},
                tBot1, n1
            });
			out.push_back({
			    {-s0 * bottomR, 0.0f, c0 * bottomR, 1.0f},
                tBot0, n0
            });
			out.push_back({
			    {-s0 * bottomR, 0.0f, c0 * bottomR, 1.0f},
                tBot0, n0
            });
			out.push_back({
			    {-s1 * topR, height, c1 * topR, 1.0f},
                tTop1, n1
            });
		}
	}

	ParticleInfo GenerateParticle() override {
		ParticleInfo p{};
		// 位置: 半径内でランダム (XZ)、高さランダム
		std::uniform_real_distribution<float> rx(preset_.posMin.x, preset_.posMax.x);
		std::uniform_real_distribution<float> ry(preset_.posMin.y, preset_.posMax.y);
		std::uniform_real_distribution<float> rz(preset_.posMin.z, preset_.posMax.z);
		std::uniform_real_distribution<float> life(preset_.lifeMin, preset_.lifeMax);
		std::uniform_real_distribution<float> colR(preset_.colMin.x, preset_.colMax.x);
		std::uniform_real_distribution<float> colG(preset_.colMin.y, preset_.colMax.y);
		std::uniform_real_distribution<float> colB(preset_.colMin.z, preset_.colMax.z);
		std::uniform_real_distribution<float> colA(preset_.colMin.w, preset_.colMax.w);
		std::uniform_real_distribution<float> velY(preset_.velMin.y, preset_.velMax.y);
		std::uniform_real_distribution<float> spiral(0.0f, 2.0f * std::numbers::pi_v<float>);
		std::uniform_real_distribution<float> radial(0.0f, 1.0f);

		// 位置（XZ は指定範囲内を使用。ここでは単純に posMin/Max の箱内）
		p.transform.translate = {rx(rng_), ry(rng_), rz(rng_)};
		p.transform.scale = {1.0f, 1.0f, 1.0f};
		p.transform.rotate = {0, 0, 0};

		// らせん + 上昇速度
		float ang = spiral(rng_);
		float r = radial(rng_) * 0.6f; // 半径制限
		TuboEngine::Math::Vector3 horiz{std::cos(ang) * r, 0.0f, std::sin(ang) * r};
		p.velocity = {horiz.x * 0.4f, velY(rng_), horiz.z * 0.4f};

		p.color = {colR(rng_), colG(rng_), colB(rng_), colA(rng_)};
		p.lifeTime = life(rng_);
		p.currentTime = 0.0f;
		return p;
	}
};