#include "Animation.h"

namespace TuboEngine {

Animation LoadAnimation(const std::string& directoryPath, const std::string& filename) {
	Animation animation;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath, 0);                                     // Assimpを使ってファイルを読み込む
	assert(scene->mNumAnimations != 0);                                                        // アニメーションが存在することを確認
	aiAnimation* animationAssimp = scene->mAnimations[0];                                      // 最初のアニメーションを取得
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond); // アニメーションの総時間を設定

	// NodeAnimationの解析
	// assimpでは個々のNodeのAnimationをChannelと呼んでいるのでChannelを回してNodeAnimationの情報を取ってくる
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		TuboEngine::NodeAnimation nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()]; // Node名でNodeAnimationを取得

		// 位置のアニメーションカーブを設定（右手→左手変換）
		for (uint32_t i = 0; i < nodeAnimationAssimp->mNumPositionKeys; ++i) {
			TuboEngine::KeyFrame<TuboEngine::Math::Vector3> keyFrame;
			keyFrame.time = float(nodeAnimationAssimp->mPositionKeys[i].mTime / animationAssimp->mTicksPerSecond);
			keyFrame.value = TuboEngine::Math::Vector3(
			    -nodeAnimationAssimp->mPositionKeys[i].mValue.x, // x反転
			    nodeAnimationAssimp->mPositionKeys[i].mValue.y, nodeAnimationAssimp->mPositionKeys[i].mValue.z);
			nodeAnimation.translate.keyframes.push_back(keyFrame);
		}
		// 回転のアニメーションカーブを設定（必要なら左右変換）
		for (uint32_t i = 0; i < nodeAnimationAssimp->mNumRotationKeys; ++i) {
			TuboEngine::KeyFrame<TuboEngine::Math::Quaternion> keyFrame;
			keyFrame.time = float(nodeAnimationAssimp->mRotationKeys[i].mTime / animationAssimp->mTicksPerSecond);
			// x, y, z反転（右手→左手変換。必要に応じて調整）
			keyFrame.value = TuboEngine::Math::Quaternion(
			    nodeAnimationAssimp->mRotationKeys[i].mValue.w, -nodeAnimationAssimp->mRotationKeys[i].mValue.x, nodeAnimationAssimp->mRotationKeys[i].mValue.y,
			    nodeAnimationAssimp->mRotationKeys[i].mValue.z);
			nodeAnimation.rotate.keyframes.push_back(keyFrame);
		}
		// スケールのアニメーションカーブを設定
		for (uint32_t i = 0; i < nodeAnimationAssimp->mNumScalingKeys; ++i) {
			TuboEngine::KeyFrame<TuboEngine::Math::Vector3> keyFrame;
			keyFrame.time = float(nodeAnimationAssimp->mScalingKeys[i].mTime / animationAssimp->mTicksPerSecond);
			keyFrame.value = TuboEngine::Math::Vector3(nodeAnimationAssimp->mScalingKeys[i].mValue.x, nodeAnimationAssimp->mScalingKeys[i].mValue.y, nodeAnimationAssimp->mScalingKeys[i].mValue.z);
			nodeAnimation.scale.keyframes.push_back(keyFrame);
		}
		// ノード名をキーにしてNodeAnimationを保存
		std::string nodeName(nodeAnimationAssimp->mNodeName.C_Str());
		animation.nodeAnimations[nodeName] = nodeAnimation;
	}
	// 解析完了
	return animation;
}

std::vector<std::string> GetAnimationNodeNames(const std::string& directoryPath, const std::string& filename) {
	std::vector<std::string> nodeNames;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath, 0);
	assert(scene->mNumAnimations != 0);
	aiAnimation* animationAssimp = scene->mAnimations[0];
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		nodeNames.push_back(nodeAnimationAssimp->mNodeName.C_Str());
	}
	return nodeNames;
}

} // namespace TuboEngine
