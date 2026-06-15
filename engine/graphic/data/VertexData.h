#pragma once

#include"Vector2.h"
#include"Vector3.h"
#include"Vector4.h"

namespace TuboEngine {
struct VertexData {
	TuboEngine::Math::Vector4 position;
	TuboEngine::Math::Vector2 texcoord;
	TuboEngine::Math::Vector3 normal;
};
} // namespace TuboEngine