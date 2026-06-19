#pragma once
#include "IParticleEmitter.h"
#include <cmath>
#include <numbers>

class CylinderEmitter : public IParticleEmitter {
public:
	void Initialize(const ParticlePreset& preset) override {
		// 本来の Cylinder は縦ストリークのテクスチャ(gradationLine)を円筒側面に貼って使う。
		// テクスチャ未指定なら既定の縦ストリークを補う。
		ParticlePreset p = preset;
		if (p.texture.empty()) {
			p.texture = "gradationLine.png";
		}
		// 円柱は3Dメッシュなのでビルボード厳禁。ON のままだとメッシュ全体がカメラを向いて
		// 潰れ、円柱に見えなくなる（必ず false にする）。
		p.billboard = false;
		IParticleEmitter::Initialize(p);
	}

	// 縦軸(Y)まわりに回転させてストリークを渦巻かせる。
	// UpdateParticles は rotate を上書きしないので、ここで加算した回転は保持される。
	void Update(float dt, const TuboEngine::Camera* camera) override {
		for (auto& particle : particles_) {
			particle.transform.rotate.y += spinSpeedY_ * dt;
		}
		IParticleEmitter::Update(dt, camera);
	}

	// 回転速度（ラジアン/秒）。0 で停止。符号で回転方向。
	void SetSpinSpeedY(float radPerSec) { spinSpeedY_ = radPerSec; }
	float GetSpinSpeedY() const { return spinSpeedY_; }

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

			// 円筒の上端=テクスチャ下端(v=1)、下端=テクスチャ上端(v=0)。
			// （V を反転して貼り、ストリークの向きを上下逆にする）
			TuboEngine::Math::Vector2 tTop0{u0, 1.0f}, tTop1{u1, 1.0f};
			TuboEngine::Math::Vector2 tBot0{u0, 0.0f}, tBot1{u1, 0.0f};

			TuboEngine::Math::Vector3 n0{-s0, 0.0f, c0};
			TuboEngine::Math::Vector3 n1{-s1, 0.0f, c1};

			// 四角形(top0,top1,bot1,bot0)を三角形2枚で隙間なく埋める。
			// 旧コードは2枚目が {bot0,bot0,top1} と頂点重複の縮退三角形になっていて
			// 側面に穴が開いていた（円柱に見えない原因）。
			// 三角形1: top0, top1, bot1
			out.push_back({ {-s0 * topR,    height, c0 * topR,    1.0f}, tTop0, n0 });
			out.push_back({ {-s1 * topR,    height, c1 * topR,    1.0f}, tTop1, n1 });
			out.push_back({ {-s1 * bottomR, 0.0f,   c1 * bottomR, 1.0f}, tBot1, n1 });
			// 三角形2: top0, bot1, bot0
			out.push_back({ {-s0 * topR,    height, c0 * topR,    1.0f}, tTop0, n0 });
			out.push_back({ {-s1 * bottomR, 0.0f,   c1 * bottomR, 1.0f}, tBot1, n1 });
			out.push_back({ {-s0 * bottomR, 0.0f,   c0 * bottomR, 1.0f}, tBot0, n0 });
		}
	}

	ParticleInfo GenerateParticle() override {
		// 本来の Cylinder: spiral 等は足さず、preset の値で素直に置く静止円筒。
		// （縦ストリークのテクスチャを円筒側面に貼って表現する）
		ParticleInfo p{};
		auto uni = [&](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng_); };

		p.transform.scale = preset_.scaleStart;
		p.transform.rotate = {0.0f, 0.0f, 0.0f};
		p.transform.translate = preset_.center + TuboEngine::Math::Vector3{
		    uni(preset_.posMin.x, preset_.posMax.x),
		    uni(preset_.posMin.y, preset_.posMax.y),
		    uni(preset_.posMin.z, preset_.posMax.z)};

		p.velocity = {
		    uni(preset_.velMin.x, preset_.velMax.x),
		    uni(preset_.velMin.y, preset_.velMax.y),
		    uni(preset_.velMin.z, preset_.velMax.z)};

		p.color = preset_.colorStart;
		p.lifeTime = uni(preset_.lifeMin, preset_.lifeMax);
		p.currentTime = 0.0f;
		return p;
	}

private:
	float spinSpeedY_ = 2.0f; // 縦軸まわりの回転速度（ラジアン/秒）
};