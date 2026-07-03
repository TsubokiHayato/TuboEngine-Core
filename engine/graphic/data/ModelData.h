#pragma once
#include"MaterialData.h"
#include"VertexData.h"
#include<vector>
#include"Node.h"

namespace TuboEngine {
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
};
} // namespace TuboEngine