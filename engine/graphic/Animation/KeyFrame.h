#pragma once
#include "Vector3.h"
#include"Quaternion.h"

namespace TuboEngine {
/// <summary>
/// キーフレーム1個分のデータ（時刻と値）。
/// </summary>
template<typename tValue> struct KeyFrame {

	float time = 0.0f; // キーフレームの時間
	tValue value;      // キーフレームの値
};

using KeyFrameVector3 = KeyFrame<TuboEngine::Math::Vector3>;
using KeyFrameQuaternion = KeyFrame<TuboEngine::Math::Quaternion>;

} // namespace TuboEngine