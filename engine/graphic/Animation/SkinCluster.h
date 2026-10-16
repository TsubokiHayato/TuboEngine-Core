#pragma once
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "ModelData.h"
#include "Skeleton.h"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace TuboEngine {
/// <summary>
/// 1頂点が受けられるジョイントの最大数。
/// </summary>
constexpr uint32_t kNumMaxInfluence = 4;

/// <summary>
/// 1頂点分のインフルエンス（頂点バッファのスロット1に流す）。
/// </summary>
struct VertexInfluence {
	std::array<float, kNumMaxInfluence> weights;        // 各ジョイントの影響度（合計1）
	std::array<int32_t, kNumMaxInfluence> jointIndices; // 影響するジョイントのインデックス
};

/// <summary>
/// マトリックスパレット1要素分のGPU転送用データ。
/// </summary>
struct WellForGPU {
	TuboEngine::Math::Matrix4x4 skeletonSpaceMatrix;                 // 位置用
	TuboEngine::Math::Matrix4x4 skeletonSpaceInverseTransposeMatrix; // 法線用
};

/// <summary>
/// スキニングに必要なGPUリソース一式（オブジェクトごとに持つ）。
/// </summary>
struct SkinCluster {
	// ジョイントごとのバインドポーズ逆行列
	std::vector<TuboEngine::Math::Matrix4x4> inverseBindPoseMatrices;

	// インフルエンス（頂点ごと）
	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView{};
	std::span<VertexInfluence> mappedInfluence;

	// マトリックスパレット（ジョイントごと、VS の StructuredBuffer）
	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
};

/// <summary>
/// スケルトンとモデルのスキニング情報から SkinCluster を作成する。
/// </summary>
SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData);

/// <summary>
/// 現在のスケルトンの姿勢からマトリックスパレットを更新する。
/// </summary>
void UpdateSkinCluster(SkinCluster& skinCluster, const Skeleton& skeleton);
} // namespace TuboEngine
