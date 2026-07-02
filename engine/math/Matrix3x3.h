#pragma once

namespace TuboEngine::Math {
/// <summary>
/// 3x3行列。2D変換などに使用する。
/// </summary>
struct Matrix3x3 final {
	float m[3][3];
};
} // namespace TuboEngine::Math