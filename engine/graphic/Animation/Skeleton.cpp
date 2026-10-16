#include "Skeleton.h"
#include "Matrix.h"

namespace TuboEngine {

namespace {
/// <summary>
/// ノードから再帰的にジョイントを作成し、作成したジョイントのインデックスを返す。
/// </summary>
/// <summary>
/// 1ジョイント分の姿勢をアニメーションから求める（カーブが無い要素はレストポーズ）。
/// </summary>
QuaternionTransform SampleJoint(const Joint& joint, const Animation& animation, float animationTime) {
	QuaternionTransform result = joint.restTransform;
	auto it = animation.nodeAnimations.find(joint.name);
	if (it == animation.nodeAnimations.end()) {
		return result; // このジョイントのアニメーションは無い
	}
	const NodeAnimation& nodeAnimation = it->second;
	if (!nodeAnimation.translate.keyframes.empty()) {
		result.translate = CalculateValue(nodeAnimation.translate.keyframes, animationTime);
	}
	if (!nodeAnimation.rotate.keyframes.empty()) {
		result.rotate = CalculateValue(nodeAnimation.rotate.keyframes, animationTime);
	}
	if (!nodeAnimation.scale.keyframes.empty()) {
		result.scale = CalculateValue(nodeAnimation.scale.keyframes, animationTime);
	}
	return result;
}

int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {
	Joint joint;
	joint.name = node.name;
	joint.transform = node.transform;
	joint.restTransform = node.transform;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = TuboEngine::Math::MakeIdentity4x4();
	joint.index = int32_t(joints.size());
	joint.parent = parent;
	joints.push_back(joint); // 子より先に親を登録する
	for (const Node& child : node.children) {
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}
	return joint.index;
}
} // namespace

Skeleton CreateSkeleton(const Node& rootNode) {
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	// 名前とインデックスの対応表を作る
	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	UpdateSkeleton(skeleton);
	return skeleton;
}

void UpdateSkeleton(Skeleton& skeleton) {
	// joints は親が先に並んでいるので先頭から順に計算すればよい
	for (Joint& joint : skeleton.joints) {
		joint.localMatrix = TuboEngine::Math::MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*joint.parent].skeletonSpaceMatrix;
		} else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime) {
	for (Joint& joint : skeleton.joints) {
		joint.transform = SampleJoint(joint, animation, animationTime);
	}
}

void ApplyAnimationBlend(Skeleton& skeleton, const Animation& from, float fromTime, const Animation& to, float toTime, float weight) {
	for (Joint& joint : skeleton.joints) {
		QuaternionTransform a = SampleJoint(joint, from, fromTime);
		QuaternionTransform b = SampleJoint(joint, to, toTime);
		joint.transform.translate = TuboEngine::Math::Vector3::Lerp(a.translate, b.translate, weight);
		joint.transform.rotate = TuboEngine::Math::Quaternion::Slerp(a.rotate, b.rotate, weight);
		joint.transform.scale = TuboEngine::Math::Vector3::Lerp(a.scale, b.scale, weight);
	}
}

Animation CapturePose(const Skeleton& skeleton, const std::string& name) {
	Animation pose;
	pose.name = name;
	pose.duration = 1.0f; // 1フレームでも再生対象になるよう長さを持たせる
	for (const Joint& joint : skeleton.joints) {
		NodeAnimation& nodeAnimation = pose.nodeAnimations[joint.name];
		nodeAnimation.translate.keyframes.push_back({0.0f, joint.transform.translate});
		nodeAnimation.rotate.keyframes.push_back({0.0f, joint.transform.rotate});
		nodeAnimation.scale.keyframes.push_back({0.0f, joint.transform.scale});
	}
	return pose;
}

} // namespace TuboEngine
