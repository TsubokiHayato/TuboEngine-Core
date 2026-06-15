#pragma once
#include "IParticleEmitter.h"
#include <numbers>

// プレイヤー移動時のみ足跡トレイルを生成するエミッター。
// ライフタイムに応じて preset.colorStart -> preset.colorEnd へ補間（IParticleEmitter 側で実施）。
class OrbitTrailEmitter : public IParticleEmitter {
public:
    void Initialize(const ParticlePreset& preset) override {
        IParticleEmitter::Initialize(preset);
        prevCenter_ = preset_.center;
    }

    void SetCenter(const TuboEngine::Math::Vector3& center) { preset_.center = center; }

    // 移動していない場合は Emit を抑制
    void Emit(uint32_t count) override {
		TuboEngine::Math::Vector3 d{preset_.center.x - prevCenter_.x, preset_.center.y - prevCenter_.y, preset_.center.z - prevCenter_.z};
        float lenSq = d.x*d.x + d.y*d.y + d.z*d.z;
        if (lenSq < moveThresholdSq_) { return; }
        IParticleEmitter::Emit(count);
    }

protected:
	void BuildGeometry(std::vector<TuboEngine::VertexData>& outVertices) override {
        outVertices.clear();
		outVertices.push_back(
		    TuboEngine::VertexData{
		        .position = {1, 1, 0, 1},
                  .texcoord = {0, 0},
                  .normal = {0, 0, 1}
        });
		outVertices.push_back(
		    TuboEngine::VertexData{
		        .position = {-1, 1, 0, 1},
                  .texcoord = {1, 0},
                  .normal = {0, 0, 1}
        });
		outVertices.push_back(
		    TuboEngine::VertexData{
		        .position = {1, -1, 0, 1},
                  .texcoord = {0, 1},
                  .normal = {0, 0, 1}
        });
		outVertices.push_back(
		    TuboEngine::VertexData{
		        .position = {1, -1, 0, 1},
                  .texcoord = {0, 1},
                  .normal = {0, 0, 1}
        });
		outVertices.push_back(
		    TuboEngine::VertexData{
		        .position = {-1, 1, 0, 1},
                  .texcoord = {1, 0},
                  .normal = {0, 0, 1}
        });
		outVertices.push_back(
		    TuboEngine::VertexData{
		        .position = {-1, -1, 0, 1},
                  .texcoord = {1, 1},
                  .normal = {0, 0, 1}
        });
    }

    ParticleInfo GenerateParticle() override {
        ParticleInfo p{};
        p.transform.scale = preset_.scaleStart;
        p.transform.rotate = {0,0,0};
        p.transform.translate = preset_.center;
        p.velocity = {0.0f, 0.2f, 0.0f};
        // 寿命はプリセット範囲から
        std::uniform_real_distribution<float> lifeDist(preset_.lifeMin, preset_.lifeMax);
        p.lifeTime = lifeDist(rng_);
        p.currentTime = 0.0f;
        // 初期色は開始色（経過で IParticleEmitter 側が colorStart->colorEnd に補間）
        p.color = preset_.colorStart;
        return p;
    }

    void Update(float dt, const TuboEngine::Camera* camera) override {
        IParticleEmitter::Update(dt, camera);
        prevCenter_ = preset_.center;
    }

private:
	TuboEngine::Math::Vector3 prevCenter_{};
    float moveThresholdSq_ = 0.0001f;
};
