#pragma once
#include "Vector3.h"

namespace TuboEngine {
/// <summary>
/// 拡縮・回転・平行移動をまとめたトランスフォーム。
/// </summary>
struct Transform {
	// 拡大率
	TuboEngine::Math::Vector3 scale;
	// 回転
	TuboEngine::Math::Vector3 rotate;
	// 平行移動
	TuboEngine::Math::Vector3 translate;
};
} // namespace TuboEngine