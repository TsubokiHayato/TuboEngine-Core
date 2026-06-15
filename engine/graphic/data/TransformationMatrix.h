#pragma once
#include "Matrix4x4.h"

namespace TuboEngine {
// 変換行列
struct TransformationMatrix {
	// ワールドビュープロジェクション行列
	TuboEngine::Math::Matrix4x4 WVP;
	// ワールド行列
	TuboEngine::Math::Matrix4x4 World;
};
} // namespace TuboEngine