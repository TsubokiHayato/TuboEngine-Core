#pragma once
#include <string>
#include <vector>
#include"Matrix4x4.h"


namespace TuboEngine {
/// ノード構造体
struct Node {
	Matrix4x4 localMatrix;      // ローカル行列
	std::string name;           // ノード名
	std::vector<Node> children; // 子ノード
};
} // namespace TuboEngine