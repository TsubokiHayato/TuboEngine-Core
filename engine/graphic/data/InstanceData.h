#pragma once
#include <Matrix4x4.h>
#include <Vector4.h>

namespace TuboEngine {
// インスタンス描画用のデータ構造
struct InstanceData {
	TuboEngine::Math::Matrix4x4 WVP;
	TuboEngine::Math::Matrix4x4 World;
	TuboEngine::Math::Vector4 Color;
};
}
