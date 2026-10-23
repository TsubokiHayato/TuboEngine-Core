#include "Animation.h"
#include <assimp/Importer.hpp> // Assimpのインポーターを使用するために必要
#include <assimp/anim.h>       // Assimpのアニメーションデータを使用するために必要
#include <assimp/scene.h>      // Assimpのシーンデータを使用するために必要
#include <cassert>
#include <format>
#include <map>
#include "Logger.h"

namespace TuboEngine {

namespace {
/// <summary>
/// assimp のアニメーション1本をエンジンの形式に変換する（右手→左手変換込み）。
/// </summary>
Animation ConvertAnimation(const aiAnimation* animationAssimp) {
	Animation animation;
	animation.name = animationAssimp->mName.C_Str();
	// mTicksPerSecond が 0 のファイルもあるので 25 (assimp の既定値) で補う
	double ticksPerSecond = animationAssimp->mTicksPerSecond != 0.0 ? animationAssimp->mTicksPerSecond : 25.0;
	animation.duration = float(animationAssimp->mDuration / ticksPerSecond); // アニメーションの総時間を設定

	// NodeAnimationの解析
	// assimpでは個々のNodeのAnimationをChannelと呼んでいるのでChannelを回してNodeAnimationの情報を取ってくる
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		// 位置のアニメーションカーブを設定（右手→左手変換: x反転）
		for (uint32_t i = 0; i < nodeAnimationAssimp->mNumPositionKeys; ++i) {
			const aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[i];
			KeyFrameVector3 keyFrame;
			keyFrame.time = float(keyAssimp.mTime / ticksPerSecond);
			keyFrame.value = {-keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z};
			nodeAnimation.translate.keyframes.push_back(keyFrame);
		}
		// 回転のアニメーションカーブを設定（右手→左手変換: y,z反転）
		for (uint32_t i = 0; i < nodeAnimationAssimp->mNumRotationKeys; ++i) {
			const aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[i];
			KeyFrameQuaternion keyFrame;
			keyFrame.time = float(keyAssimp.mTime / ticksPerSecond);
			keyFrame.value = TuboEngine::Math::Quaternion(keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w);
			nodeAnimation.rotate.keyframes.push_back(keyFrame);
		}
		// スケールのアニメーションカーブを設定
		for (uint32_t i = 0; i < nodeAnimationAssimp->mNumScalingKeys; ++i) {
			const aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[i];
			KeyFrameVector3 keyFrame;
			keyFrame.time = float(keyAssimp.mTime / ticksPerSecond);
			keyFrame.value = {keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z};
			nodeAnimation.scale.keyframes.push_back(keyFrame);
		}
	}
	// 解析完了
	return animation;
}
} // namespace

namespace {
// 読み込み済みアニメーション（ファイルパス → ファイル内の全アニメーション）。
// 同じ glTF を何度も Assimp でパースしないためのキャッシュ。
std::map<std::string, std::vector<Animation>>& AnimationCache() {
	static std::map<std::string, std::vector<Animation>> cache;
	return cache;
}

// ファイル内の全アニメーションを返す（2回目以降はキャッシュから）。読めなければ空。
const std::vector<Animation>& LoadAnimationsCached(const std::string& directoryPath, const std::string& filename) {
	const std::string filePath = directoryPath + "/" + filename;
	auto& cache = AnimationCache();
	if (auto it = cache.find(filePath); it != cache.end()) {
		return it->second;
	}

	std::vector<Animation> animations;
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath, 0); // Assimpを使ってファイルを読み込む
	if (!scene) {
		// パス間違いなど。落とさずにログを出して「アニメーション無し」として扱う
		// （失敗結果もキャッシュする＝同じエラーを毎回出さない）
		Logger::Log(std::format("[Animation] 読み込み失敗: {} ({})\n", filePath, importer.GetErrorString()));
	} else {
		for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
			animations.push_back(ConvertAnimation(scene->mAnimations[i]));
		}
	}
	return cache.emplace(filePath, std::move(animations)).first->second;
}
} // namespace

Animation LoadAnimation(const std::string& directoryPath, const std::string& filename, uint32_t animationIndex) {
	const std::vector<Animation>& animations = LoadAnimationsCached(directoryPath, filename);
	if (animationIndex >= animations.size()) {
		return Animation{}; // アニメーションが無い（duration == 0）
	}
	return animations[animationIndex];
}

std::vector<Animation> LoadAnimations(const std::string& directoryPath, const std::string& filename) {
	return LoadAnimationsCached(directoryPath, filename);
}

void ClearAnimationCache() { AnimationCache().clear(); }

std::vector<std::string> GetAnimationNodeNames(const std::string& directoryPath, const std::string& filename) {
	std::vector<std::string> nodeNames;
	Animation animation = LoadAnimation(directoryPath, filename);
	for (const auto& [nodeName, nodeAnimation] : animation.nodeAnimations) {
		nodeNames.push_back(nodeName);
	}
	return nodeNames;
}

// Vector3
TuboEngine::Math::Vector3 CalculateValue(const std::vector<KeyFrameVector3>& keyframes, float time) {
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}
	for (size_t i = 0; i < keyframes.size() - 1; ++i) {
		size_t next = i + 1;
		if (time < keyframes[next].time) {
			float t = (time - keyframes[i].time) / (keyframes[next].time - keyframes[i].time);
			return TuboEngine::Math::Vector3::Lerp(keyframes[i].value, keyframes[next].value, t);
		}
	}
	return keyframes.back().value;
}

// Quaternion
TuboEngine::Math::Quaternion CalculateValue(const std::vector<KeyFrameQuaternion>& keyframes, float time) {
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}
	for (size_t i = 0; i < keyframes.size() - 1; ++i) {
		size_t next = i + 1;
		if (time < keyframes[next].time) {
			float t = (time - keyframes[i].time) / (keyframes[next].time - keyframes[i].time);
			// QuaternionのSlerp（球面線形補間）
			return TuboEngine::Math::Quaternion::Slerp(keyframes[i].value, keyframes[next].value, t);
		}
	}
	return keyframes.back().value;
}

} // namespace TuboEngine
