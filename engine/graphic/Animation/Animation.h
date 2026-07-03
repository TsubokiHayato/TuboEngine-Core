#pragma once
#include "KeyFrame.h"
#include <Vector3.h>
#include <map>
#include <string>
#include <vector>
#include <assimp/scene.h> // Assimpのシーンデータを使用するために必要
#include <assimp/anim.h> // Assimpのアニメーションデータを使用するために必要
#include <assimp/Importer.hpp> // Assimpのインポーターを使用するために必要
#include"assert.h"

namespace TuboEngine {
/// <summary>
/// キーフレームの配列で表すアニメーションカーブ。
/// </summary>
template<typename tValue> struct AnimationCurve {
	std::vector<TuboEngine::KeyFrame<tValue>> keyframes; // キーフレームのリスト
};

/// <summary>
/// ノード1個分のアニメーション（平行移動・回転・スケールの各カーブ）。
/// </summary>
struct NodeAnimation {
	AnimationCurve<TuboEngine::Math::Vector3> translate; // 位置のアニメーションカーブ
	AnimationCurve<TuboEngine::Math::Quaternion> rotate; // 回転のアニメーションカーブ
	AnimationCurve<TuboEngine::Math::Vector3> scale;     // スケールのアニメーションカーブ
};

/// <summary>
/// アニメーション1本分のデータ（全体の長さと各ノードのカーブ）。
/// </summary>
struct Animation {
	float duration = 0.0f; // アニメーションの総時間
	// NodeAnimationの集合。Node名で引けるようにしておく
	std::map<std::string, NodeAnimation> nodeAnimations; // ノードごとのアニメーション
};

Animation LoadAnimation(const std::string& directoryPath, const std::string& filename);
std::vector<std::string> GetAnimationNodeNames(const std::string& directoryPath, const std::string& filename);
} // namespace MyNamespace