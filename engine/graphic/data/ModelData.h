#pragma once
#include"MaterialData.h"
#include"VertexData.h"
#include<vector>
#include"Node.h"
#include<cstdint>
#include<map>
#include<string>

namespace TuboEngine {
/// <summary>
/// ジョイントが1頂点に与える影響（ウェイト）。
/// </summary>
struct VertexWeightData {
	float weight;         // 影響度
	uint32_t vertexIndex; // ModelData::vertices のインデックス
};

/// <summary>
/// ジョイント1個分のスキニング情報。
/// </summary>
struct JointWeightData {
	TuboEngine::Math::Matrix4x4 inverseBindPoseMatrix; // バインドポーズの逆行列
	std::vector<VertexWeightData> vertexWeights;       // 影響を与える頂点
};

/// <summary>
/// モデルデータ（頂点・マテリアル）。
/// </summary>
struct ModelData {
	// 頂点データ
	std::vector<VertexData> vertices;
	// マテリアルデータ
	MaterialData material;
	// ノードデータ
	Node rootNode;
	// メッシュを持つ最初のノード名（ノードアニメーションの適用先）
	std::string meshNodeName;
	// ジョイント名 → スキニング情報。空ならスキニングなし
	std::map<std::string, JointWeightData> skinClusterData;
};
} // namespace TuboEngine