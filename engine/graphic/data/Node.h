#pragma once
#include <string>
#include <vector>
#include"Matrix4x4.h"
#include"Quaternion.h"
#include"Vector3.h"


namespace TuboEngine {
/// <summary>
/// クォータニオンで回転を表すトランスフォーム（アニメーション用）。
/// </summary>
struct QuaternionTransform {
	TuboEngine::Math::Vector3 scale = {1.0f, 1.0f, 1.0f};
	TuboEngine::Math::Quaternion rotate;
	TuboEngine::Math::Vector3 translate = {0.0f, 0.0f, 0.0f};
};

/// ノード構造体
struct Node {
	QuaternionTransform transform; // ローカルトランスフォーム（左手系に変換済み）
	TuboEngine::Math::Matrix4x4 localMatrix; // ローカル行列
	std::string name;           // ノード名
	std::vector<Node> children; // 子ノード
};
} // namespace TuboEngine