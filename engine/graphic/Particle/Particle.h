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



/// <summary>
/// 旧来のレガシーなパーティクル用クラス（Emitterシステムとは別）。
/// </summary>
class Particle {
public:
	enum class ParticleType { None, Primitive, Ring, Cylinder, Original };

	/// <summary>
	/// 初期化処理。
	/// </summary>
	void Initialize(ParticleType particleType);
	/// <summary>
	/// 更新処理。
	/// </summary>
	void Update();
	/// <summary>
	/// 描画処理。
	/// </summary>
	void Draw();
	/// <summary>
	/// パーティクルを指定数発生させる。
	/// </summary>
	void Emit(const std::string name, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime, uint32_t count);
	/// <summary>
	/// ParticleGroup の生成。
	/// </summary>
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
	/// <summary>
	/// パーティクル1個を生成する（旧方式）。
	/// </summary>
	ParticleInfo CreateNewParticle(std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);
	/// <summary>
	/// 板ポリゴン用パーティクル1個を生成する（旧方式）。
	/// </summary>
	ParticleInfo CreateNewParticleForPrimitive(
	    std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);
	/// <summary>
	/// リング形状用パーティクル1個を生成する（旧方式）。
	/// </summary>
	ParticleInfo CreateNewParticleForRing(
	    std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);
	/// <summary>
	/// 円柱形状用パーティクル1個を生成する（旧方式）。
	/// </summary>
	ParticleInfo CreateNewParticleForCylinder(
	    std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);
	/// <summary>
	/// 独自形状用パーティクル1個を生成する（旧方式）。
	/// </summary>
	ParticleInfo CreateNewParticleForOriginal(
	    std::mt19937& randomEngine, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, TuboEngine::Math::Vector4 color, float lifeTime, float currentTime);

private:
	/// <summary>
	/// テクスチャ単位でパーティクルをまとめて管理するグループ。インスタンシング用リソースを保持する。
	/// </summary>
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

	/// <summary>
	/// float の最小値・最大値で表す範囲。
	/// </summary>
	struct RangeF {
		float min;
		float max;
	};

	ParticleType particleType_ = ParticleType::None;
	std::mt19937 randomEngine_;

	/// <summary>
	/// 頂点データ（旧方式）。
	/// </summary>
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
