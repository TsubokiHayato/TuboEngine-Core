#pragma once
#include <cstdint>
#include "Vector4.h"
#include "Matrix4x4.h"

using TuboEngine::Math::Matrix4x4;
using TuboEngine::Math::Vector4;

namespace TuboEngine {
// マテリアル
struct Material {
	//	色
	Vector4 color;
	// ライティングを有効にするか
	int32_t enableLighting;
	// パディング
	float padding[3];
	// UV変換行列
	Matrix4x4 uvTransform;
	// 光沢度
	float shininess;
	// 環境マップ寄与度 [0:無し, 1:全反射]
	float environmentCoefficient;
};
} // namespace TuboEngine