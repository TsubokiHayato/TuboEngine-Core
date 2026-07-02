#pragma once
#include <Matrix4x4.h>
#include <Vector4.h>

namespace TuboEngine {
/// <summary>
/// インスタンシング描画用の1インスタンス分データ。
/// </summary>
struct InstanceData {
	TuboEngine::Math::Matrix4x4 WVP;
	TuboEngine::Math::Matrix4x4 World;
	TuboEngine::Math::Vector4 Color;
};
}
