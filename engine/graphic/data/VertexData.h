#pragma once

#include"Vector2.h"
#include"Vector3.h"
#include"Vector4.h"

namespace TuboEngine {
/// <summary>
/// 頂点1個分のデータ。
/// position: 座標（同次座標）、texcoord: UV座標、normal: 法線ベクトルを保持する。
/// </summary>
struct VertexData {
	TuboEngine::Math::Vector4 position;
	TuboEngine::Math::Vector2 texcoord;
	TuboEngine::Math::Vector3 normal;
};
} // namespace TuboEngine