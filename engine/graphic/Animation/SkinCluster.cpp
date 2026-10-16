#include "SkinCluster.h"
#include "Matrix.h"
#include <algorithm>
#include <cassert>
#include <cstring>

namespace TuboEngine {

namespace {
/// <summary>
/// インフルエンスにウェイトを追加する。枠が埋まっていたら最小のものと入れ替える。
/// </summary>
void AddInfluence(VertexInfluence& influence, float weight, int32_t jointIndex) {
	auto minIt = std::min_element(influence.weights.begin(), influence.weights.end());
	if (weight > *minIt) {
		size_t slot = std::distance(influence.weights.begin(), minIt);
		influence.weights[slot] = weight;
		influence.jointIndices[slot] = jointIndex;
	}
}
} // namespace

SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData) {
	SkinCluster skinCluster;
	const size_t jointCount = skeleton.joints.size();
	const size_t vertexCount = modelData.vertices.size();
	assert(jointCount > 0 && vertexCount > 0);

	// パレット用のリソースを確保（VS からはルートSRVとして読む）
	skinCluster.paletteResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(WellForGPU) * jointCount);
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = {mappedPalette, jointCount};
	for (WellForGPU& well : skinCluster.mappedPalette) {
		well.skeletonSpaceMatrix = TuboEngine::Math::MakeIdentity4x4();
		well.skeletonSpaceInverseTransposeMatrix = TuboEngine::Math::MakeIdentity4x4();
	}

	// インフルエンス用のリソースを確保（頂点バッファのスロット1）
	skinCluster.influenceResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VertexInfluence) * vertexCount);
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * vertexCount); // weight 0 で埋めておく
	skinCluster.mappedInfluence = {mappedInfluence, vertexCount};

	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * vertexCount);
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// バインドポーズ逆行列（スキンに含まれないジョイントは単位行列）
	skinCluster.inverseBindPoseMatrices.resize(jointCount);
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), TuboEngine::Math::MakeIdentity4x4);

	// ModelData のスキニング情報をジョイントのインデックスに対応付けて埋める
	for (const auto& [jointName, jointWeight] : modelData.skinClusterData) {
		auto it = skeleton.jointMap.find(jointName);
		if (it == skeleton.jointMap.end()) {
			continue; // スケルトンに無いジョイントは無視
		}
		const int32_t jointIndex = it->second;
		skinCluster.inverseBindPoseMatrices[jointIndex] = jointWeight.inverseBindPoseMatrix;
		for (const VertexWeightData& vertexWeight : jointWeight.vertexWeights) {
			AddInfluence(skinCluster.mappedInfluence[vertexWeight.vertexIndex], vertexWeight.weight, jointIndex);
		}
	}

	// 4本に絞った結果、合計が1にならない頂点があるので正規化する
	for (VertexInfluence& influence : skinCluster.mappedInfluence) {
		float total = 0.0f;
		for (float weight : influence.weights) {
			total += weight;
		}
		if (total > 0.0f) {
			for (float& weight : influence.weights) {
				weight /= total;
			}
		}
	}

	UpdateSkinCluster(skinCluster, skeleton);
	return skinCluster;
}

void UpdateSkinCluster(SkinCluster& skinCluster, const Skeleton& skeleton) {
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		WellForGPU& well = skinCluster.mappedPalette[jointIndex];
		well.skeletonSpaceMatrix = skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		well.skeletonSpaceInverseTransposeMatrix = TuboEngine::Math::Transpose(TuboEngine::Math::Inverse(well.skeletonSpaceMatrix));
	}
}

} // namespace TuboEngine
