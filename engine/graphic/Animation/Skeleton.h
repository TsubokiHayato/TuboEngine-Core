#pragma once
#include "Animation.h"
#include "Matrix4x4.h"
#include "Node.h"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace TuboEngine {
/// <summary>
/// スケルトンを構成する関節1個分のデータ。
/// </summary>
struct Joint {
	QuaternionTransform transform;                          // ローカルトランスフォーム（アニメーションで更新される）
	QuaternionTransform restTransform;                      // レストポーズ（アニメーションが無いときの姿勢）
	TuboEngine::Math::Matrix4x4 localMatrix;                // ローカル行列
	TuboEngine::Math::Matrix4x4 skeletonSpaceMatrix;        // スケルトン空間（ルート基準）での行列
	std::string name;                                       // 名前
	std::vector<int32_t> children;                          // 子ジョイントのインデックス
	int32_t index = 0;                                      // 自身のインデックス
	std::optional<int32_t> parent;                          // 親ジョイントのインデックス（ルートなら無し）
};

/// <summary>
/// ノード階層から作るスケルトン。joints は親が必ず子より前に並ぶ。
/// </summary>
struct Skeleton {
	int32_t root = 0;                         // ルートジョイントのインデックス
	std::map<std::string, int32_t> jointMap;  // ジョイント名 → インデックス
	std::vector<Joint> joints;                // ジョイント配列
};

/// <summary>
/// ノード階層からスケルトンを作成する。
/// </summary>
Skeleton CreateSkeleton(const Node& rootNode);

/// <summary>
/// 各ジョイントの localMatrix / skeletonSpaceMatrix を transform から再計算する。
/// </summary>
void UpdateSkeleton(Skeleton& skeleton);

/// <summary>
/// 指定時刻のアニメーションをスケルトンに適用する（UpdateSkeleton の前に呼ぶ）。
/// アニメーションを持たないジョイントはレストポーズに戻る。
/// </summary>
void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime);

/// <summary>
/// 2つのアニメーションを混ぜてスケルトンに適用する（クロスフェード用）。
/// </summary>
/// <param name="weight">0 で from のみ、1 で to のみ</param>
void ApplyAnimationBlend(Skeleton& skeleton, const Animation& from, float fromTime, const Animation& to, float toTime, float weight);

/// <summary>
/// スケルトンの現在の姿勢（各ジョイントの transform）を、1フレームだけのアニメーションとして切り出す。
/// 待機ポーズなど、アニメーションを持たない姿勢を Animator に登録するのに使う。
/// </summary>
Animation CapturePose(const Skeleton& skeleton, const std::string& name);
} // namespace TuboEngine
