#pragma once
#include "KeyFrame.h"
#include <Vector3.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

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
	std::string name;      // アニメーション名
	float duration = 0.0f; // アニメーションの総時間（秒）
	// NodeAnimationの集合。Node名で引けるようにしておく
	std::map<std::string, NodeAnimation> nodeAnimations; // ノードごとのアニメーション
};

/// <summary>
/// ファイル内の指定番号のアニメーションを読み込む。存在しない・読めない場合は空（duration == 0）を返す（ログが出る）。
/// </summary>
Animation LoadAnimation(const std::string& directoryPath, const std::string& filename, uint32_t animationIndex = 0);
/// <summary>
/// ファイル内の全アニメーションを読み込む。
/// </summary>
std::vector<Animation> LoadAnimations(const std::string& directoryPath, const std::string& filename);
/// <summary>
/// 読み込み済みアニメーションのキャッシュを消す（glTF を差し替えて読み直したい時用）。
/// LoadAnimation / LoadAnimations は同じファイルを2回目以降キャッシュから返す。
/// </summary>
void ClearAnimationCache();
/// <summary>
/// 先頭アニメーションが対象とするノード名の一覧を取得する。
/// </summary>
std::vector<std::string> GetAnimationNodeNames(const std::string& directoryPath, const std::string& filename);

/// <summary>
/// アニメーションカーブから指定時刻の値を計算する（線形補間）。
/// </summary>
TuboEngine::Math::Vector3 CalculateValue(const std::vector<KeyFrameVector3>& keyframes, float time);
/// <summary>
/// アニメーションカーブから指定時刻の値を計算する（球面線形補間）。
/// </summary>
TuboEngine::Math::Quaternion CalculateValue(const std::vector<KeyFrameQuaternion>& keyframes, float time);
} // namespace TuboEngine
