#pragma once
#include "Matrix4x4.h"

namespace TuboEngine {
/// <summary>
/// 変換行列（WVP・ワールド行列）のGPU転送用データ。
/// </summary>
struct TransformationMatrix {
	// ワールドビュープロジェクション行列
	TuboEngine::Math::Matrix4x4 WVP;
	// ワールド行列
	TuboEngine::Math::Matrix4x4 World;
};
} // namespace TuboEngine