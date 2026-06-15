#pragma once
#include "IParticleEmitter.h"

class PrimitiveEmitter : public IParticleEmitter {
public:
    // public にして外部から Initialize が呼べるようにする
    void Initialize(const ParticlePreset& preset) override {
        IParticleEmitter::Initialize(preset);
        // 必要なら倍率を調整してください (例: 4.0f)
        constexpr float sizeMultiplier = 4.0f;

        // scaleStart/scaleEnd がデフォルトのままなら scaleMin/scaleMax を流用
        bool startIsDefault = (preset_.scaleStart.x == 1.0f && preset_.scaleStart.y == 1.0f && preset_.scaleStart.z == 1.0f);
        bool endIsDefault   = (preset_.scaleEnd.x   == 1.0f && preset_.scaleEnd.y   == 1.0f && preset_.scaleEnd.z   == 1.0f);

        if ((startIsDefault && endIsDefault) &&
            !(preset_.scaleMin.x == 1.0f && preset_.scaleMin.y == 1.0f && preset_.scaleMin.z == 1.0f &&
              preset_.scaleMax.x == 1.0f && preset_.scaleMax.y == 1.0f && preset_.scaleMax.z == 1.0f)) {
            preset_.scaleStart = preset_.scaleMin;
            preset_.scaleEnd   = preset_.scaleMax;
        }

        // 倍率を掛ける
        preset_.scaleStart.x *= sizeMultiplier; preset_.scaleStart.y *= sizeMultiplier; preset_.scaleStart.z *= sizeMultiplier;
        preset_.scaleEnd.x   *= sizeMultiplier; preset_.scaleEnd.y   *= sizeMultiplier; preset_.scaleEnd.z   *= sizeMultiplier;
    }

protected:
	void BuildGeometry(std::vector<TuboEngine::VertexData>& out) override {
        // シンプルな板
        out.push_back({{ 1, 1,0,1},{0,0},{0,0,1}});
        out.push_back({{-1, 1,0,1},{1,0},{0,0,1}});
        out.push_back({{ 1,-1,0,1},{0,1},{0,0,1}});
        out.push_back({{ 1,-1,0,1},{0,1},{0,0,1}});
        out.push_back({{-1, 1,0,1},{1,0},{0,0,1}});
        out.push_back({{-1,-1,0,1},{1,1},{0,0,1}});
    }
	ParticleInfo GenerateParticle() override {
		std::uniform_real_distribution<float> life(preset_.lifeMin, preset_.lifeMax);
		// 初期スケールは preset_.scaleStart を使う（UpdateParticles で補間される）
		ParticleInfo p{};
		p.transform.scale = preset_.scaleStart;
		p.transform.rotate = {0, 0, 0};

		// 変更: preset_.center をオフセットとして加算するようにした
		float rx = std::uniform_real_distribution<float>(preset_.posMin.x, preset_.posMax.x)(rng_);
		float ry = std::uniform_real_distribution<float>(preset_.posMin.y, preset_.posMax.y)(rng_);
		float rz = std::uniform_real_distribution<float>(preset_.posMin.z, preset_.posMax.z)(rng_);
		p.transform.translate = preset_.center + TuboEngine::Math::Vector3{rx, ry, rz};

        p.velocity = {
            std::uniform_real_distribution<float>(preset_.velMin.x,preset_.velMax.x)(rng_),
            std::uniform_real_distribution<float>(preset_.velMin.y,preset_.velMax.y)(rng_),
            std::uniform_real_distribution<float>(preset_.velMin.z,preset_.velMax.z)(rng_)
        };
        p.color = preset_.colorStart;
        p.lifeTime = life(rng_);
        p.currentTime = 0.0f;
        return p;
    }
};