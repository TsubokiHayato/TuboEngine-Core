#pragma once
#include"Vector3.h"
namespace TuboEngine {
/// <summary>
/// カメラ情報（ワールド座標）のGPU転送用データ。
/// </summary>
struct CameraForGPU {
	TuboEngine::Math::Vector3 worldPosition;
};
} // namespace TuboEngine