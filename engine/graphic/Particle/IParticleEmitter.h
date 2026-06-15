#pragma once
#include "DirectXCommon.h"
#include "Material.h"
#include "ParticleCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "Transform.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "VertexData.h"
#include <list>
#include"Camera.h"
#include <memory>
#include <random>
#include <string>

struct ParticleInfo {
	TuboEngine::Transform transform;
	TuboEngine::Math::Vector3 velocity;
	TuboEngine::Math::Vector4 color;
	float lifeTime;
	float currentTime;
};

struct ParticleForGPU {
	TuboEngine::Math::Matrix4x4 WVP;
	TuboEngine::Math::Matrix4x4 World;
	TuboEngine::Math::Vector4 color;
};

struct ParticlePreset {
	// 基本
	std::string name;
	std::string texture;
	uint32_t maxInstances = 128;
	bool billboard = true;

	TuboEngine::Math::Vector3 center{0,0,0};              // 追加: 発生中心
	TuboEngine::Math::Vector3 posMin{0,0,0}, posMax{0,0,0};
	TuboEngine::Math::Vector3 velMin{0,0,0}, velMax{0,0,0};
	TuboEngine::Math::Vector3 scaleMin{1,1,1}, scaleMax{1,1,1};
	TuboEngine::Math::Vector4 colMin{1,1,1,1}, colMax{1,1,1,1};
	float lifeMin = 0.5f, lifeMax = 1.5f;

	// Emit
	bool autoEmit = false;
	float _emitAccum = 0.0f; // 内部用：蓄積時間
	float emitRate = 30.0f;  // 個/秒
	uint32_t burstCount = 10;
	// 追加
	TuboEngine::Math::Vector3 gravity{0.0f,-0.5f,0.0f};
	float drag = 0.0f;                         // 速度減衰率（0～1）
	TuboEngine::Math::Vector2 rotSpeedRangeZ{0.0f,0.0f};         // 回転速度範囲(Z軸)
	TuboEngine::Math::Vector2 initialRotRangeZ{0.0f,0.0f};       // 初期回転角ランダム範囲
	TuboEngine::Math::Vector3 scaleStart{1,1,1}, scaleEnd{1,1,1};
	TuboEngine::Math::Vector4 colorStart{1,1,1,1}, colorEnd{1,1,1,0};
	int blendModeOverride = -1;                // -1 なら共通設定を使う
	bool simulateInWorldSpace = true;
	TuboEngine::Transform emitterTransform{}; // エミッター自身の座標
};

class IParticleEmitter {
public:
	// デストラクタを明示的にし、マップ解除とリソース解放を安全に行う
	virtual ~IParticleEmitter();
	virtual void Initialize(const ParticlePreset& preset);
	virtual void Update(float dt, const TuboEngine::Camera* camera);
	virtual void Draw(ID3D12GraphicsCommandList* cmd);
	virtual void Emit(uint32_t count);
	virtual ParticlePreset& GetPreset() { return preset_; }
	virtual const ParticlePreset& GetPreset() const { return preset_; }
	virtual const std::string& GetName() const { return preset_.name; }
	virtual void DrawImGui(); // 直接呼ばれない（Manager側で統合表示）。残しつつ利用可能。

	// 追加: 全粒子消去（Debug用途）
	void ClearAll() {
		particles_.clear();
		instanceCount_ = 0;
	}

	// 追加: インスタンスバッファ再確保 (maxInstances 変更対応)
	void ReallocateInstanceBufferIfNeeded();

protected:
	// 派生で粒子1個生成
	virtual ParticleInfo GenerateParticle() = 0;
	// 派生で頂点形状生成
	virtual void BuildGeometry(std::vector<TuboEngine::VertexData>& outVertices) = 0;
	void EnsureBuffers();
	void PushParticle(const ParticleInfo& p);
	void UpdateParticles(float dt, const TuboEngine::Math::Matrix4x4& viewProj, const TuboEngine::Math::Matrix4x4& billboard);
	void ResetInstances();

protected:
	ParticlePreset preset_{};
	std::list<ParticleInfo> particles_;
	std::mt19937 rng_{ std::random_device{}() };
	Microsoft::WRL::ComPtr<ID3D12Resource> vb_;
	Microsoft::WRL::ComPtr<ID3D12Resource> material_;
	TuboEngine::Material* materialPtr_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> instancing_;
	ParticleForGPU* instancingPtr_ = nullptr;
	uint32_t instanceCount_ = 0;
	uint32_t allocatedInstances_ = 0; // 現インスタンスバッファ確保数
	int textureSrvIndex_ = -1;
	int instancingSrvIndex_ = -1;
	std::vector<TuboEngine::VertexData> vertices_;
};