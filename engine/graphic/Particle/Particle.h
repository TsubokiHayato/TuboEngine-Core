#pragma once
#include "DirectXCommon.h"
#include "IParticleEmitter.h" // 重複回避のため追加
#include "Material.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "Transform.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "VertexData.h"
#include <list>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>



// 古いレガシー方式パーティクル用クラス（Emitterシステムとは別）
class Particle {
public:
	enum class ParticleType { None, Primitive, Ring, Cylinder, Original };

	void Initialize(ParticleType particleType);
	void Update();
	void Draw();
	void Emit(const std::string name, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime, uint32_t count);
	void CreateParticleGroup(const std::string& name, const std::string& textureFilePath);

	//明示解放用デストラクタ（カメラ delete と各種 Unmap を実行）
	~Particle();

private:
	// 頂点生成
	void CreateVertexData();
	void CreateVertexDataForRing();
	void CreateVertexDataForCylinder();
	void CreateVertexDataForOriginal();
	void CreateVertexBufferView();
	void CreateMaterialData();

	// 旧パーティクル生成関数（新Emitterとは別物）
	ParticleInfo
	    CreateNewParticle(std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);
	ParticleInfo CreateNewParticleForPrimitive(
	    std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);
	ParticleInfo CreateNewParticleForRing(
	    std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);
	ParticleInfo CreateNewParticleForCylinder(
	    std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);
	ParticleInfo CreateNewParticleForOriginal(
	    std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);

private:
	struct ParticleGroup {
		std::string materialFilePath;
		std::list<ParticleInfo> particleList;
		uint32_t instanceCount = 0;
		TuboEngine::Math::Vector2 textureLeftTop{0, 0};
		TuboEngine::Math::Vector2 textureSize{1, 1};
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
		ParticleForGPU* instancingDataPtr = nullptr;
		int srvIndex = -1;
		int instancingSrvIndex = -1;
	};

	struct RangeF {
		float min;
		float max;
	};

	ParticleType particleType_ = ParticleType::None;
	std::mt19937 randomEngine_;

	// 頂点データ（旧）
	struct ModelData {
		std::vector<TuboEngine::VertexData> vertices;
	} modelData_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	TuboEngine::VertexData* vertexData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_;
	TuboEngine::Material* materialData_ = nullptr;

	std::unordered_map<std::string, ParticleGroup> particleGroups;

	std::unique_ptr<TuboEngine::Camera> camera_;

	// カスタムサイズ指定（>0 のときテクスチャサイズを上書き）
	TuboEngine::Math::Vector2 customTextureSize{0, 0};

	// 乱数範囲（プリミティブ用）
	RangeF rotateRange_{0, 0};
	RangeF scaleRange_{1, 1};

	// 定数
	static constexpr uint32_t kNumMaxInstance = 1024;
	static constexpr float kDeltaTime = 1.0f / 60.0f;
	bool isBillBoard = true;
};
