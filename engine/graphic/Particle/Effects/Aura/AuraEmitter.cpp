#include "engine/graphic/Particle/Effects/Aura/AuraEmitter.h"

void AuraEmitter::Initialize(const ParticlePreset& preset) {
	// 受け取った preset をベースに、オーラ用の既定値を上書きしてから初期化する
	ParticlePreset p = preset;

	if (p.name.empty())
		p.name = "PlayerAura";
	if (p.texture.empty())
		p.texture = "glow.png"; // 適切なテクスチャ名をプロジェクトに合わせて変更してください
	p.maxInstances = std::max<uint32_t>(p.maxInstances, 256u);
	p.autoEmit = true;
	p.emitRate = (p.emitRate > 0.0f ? p.emitRate : 10.0f); // 秒あたり
	p.lifeMin = 0.8f;
	p.lifeMax = 1.4f;
	p.scaleStart = {0.6f, 0.6f, 0.6f};
	p.scaleEnd = {1.6f, 1.6f, 1.6f};
	p.colorStart = {0.2f, 0.7f, 1.0f, 0.6f}; // シアン寄りの発光
	p.colorEnd = {0.9f, 0.3f, 1.0f, 0.0f};   // フェードアウト
	p.gravity = {0.0f, 0.0f, 0.0f};
	p.drag = 0.2f;
	p.simulateInWorldSpace = true;
	p.billboard = true;

	// PrimitiveEmitter::Initialize に委譲
	PrimitiveEmitter::Initialize(p);
}

ParticleInfo AuraEmitter::GenerateParticle() {
	// 基本は PrimitiveEmitter の実装に任せ、少しだけ速度・回転を調整する
	ParticleInfo p = PrimitiveEmitter::GenerateParticle();

	// 少しだけ外向きに広がるようにランダム速度を追加
	std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
	float rx = dir(rng_);
	float ry = dir(rng_);
	float rz = dir(rng_);
	// 正規化っぽく扱ってスケール
	float len = std::max(1e-6f, std::sqrt(rx * rx + ry * ry + rz * rz));
	float outwardStrength = 0.12f; // 微弱な外向きの広がり
	p.velocity.x += (rx / len) * outwardStrength;
	p.velocity.y += (ry / len) * outwardStrength;
	p.velocity.z += (rz / len) * outwardStrength;

	// 回転（Z軸）のランダム値を少し付与
	std::uniform_real_distribution<float> ang(-3.14159f, 3.14159f);
	p.transform.rotate.z = ang(rng_);

	return p;
}