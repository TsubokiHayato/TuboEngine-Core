#include "Animator.h"
#include "ImGuiManager.h"
#include "Matrix.h"
#include "ModelManager.h"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace {
// モデル・アニメーションを置くディレクトリ（ModelManager と揃える）
const std::string kModelDirectory = "Resources/Models";
} // namespace

// 初期化
void TuboEngine::Animator::Initialize(const std::string& modelFileNamePath) {
	// 描画は Object3d に任せる（ライト・カメラも Object3d と共通）
	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(modelFileNamePath);
	model_ = object3d_->GetModel();
	assert(model_);

	// スケルトンを作成
	const ModelData& modelData = model_->GetModelData();
	skeleton_ = CreateSkeleton(modelData.rootNode);
	auto it = skeleton_.jointMap.find(modelData.meshNodeName);
	meshJointIndex_ = it != skeleton_.jointMap.end() ? it->second : skeleton_.root;

	// スキンを持つモデルならスキニング用リソースを作成
	if (model_->IsSkinned()) {
		skinCluster_ = std::make_unique<SkinCluster>(CreateSkinCluster(skeleton_, modelData));
		object3d_->SetSkinCluster(skinCluster_.get());
	}

	// 同じファイルに含まれるアニメーションを全て登録し、先頭を再生
	std::vector<Animation> animations = TuboEngine::LoadAnimations(kModelDirectory, modelFileNamePath);
	for (size_t i = 0; i < animations.size(); ++i) {
		std::string name = animations[i].name.empty() ? "Animation" + std::to_string(i) : animations[i].name;
		AddAnimation(name, animations[i]);
		if (i == 0) {
			PlayAnimation(name, 0.0f);
		}
	}
}

float TuboEngine::Animator::AdvanceTime(float time, float deltaTime, float duration) const {
	time += deltaTime * speed_;
	if (isLoop_) {
		time = std::fmod(time, duration);
		if (time < 0.0f) {
			time += duration; // 逆再生時
		}
	} else {
		time = std::clamp(time, 0.0f, duration);
	}
	return time;
}

void TuboEngine::Animator::Update(float deltaTime) {
	const float duration = current_->duration;

	// 再生時間を進める
	if (isPlaying_) {
		if (duration > 0.0f) {
			const float previousTime = animationTime_;
			const float rawTime = animationTime_ + deltaTime * speed_; // ループで巻き戻す前の時間
			animationTime_ = AdvanceTime(animationTime_, deltaTime, duration);
			FireEvents(previousTime, animationTime_, rawTime);
			// ループしない再生が終端に着いたら1回だけ知らせる
			if (IsFinished() && !finishedNotified_) {
				finishedNotified_ = true;
				if (onFinished_) {
					onFinished_(currentName_);
				}
			}
		}
		if (previous_) {
			if (previous_->duration > 0.0f) {
				previousTime_ = AdvanceTime(previousTime_, deltaTime, previous_->duration);
			}
			blendElapsed_ += deltaTime;
			if (blendElapsed_ >= blendTime_) {
				previous_ = nullptr; // クロスフェード終了
			}
		}
	}

	// スケルトンの姿勢を更新
	if (previous_) {
		float weight = std::clamp(blendElapsed_ / blendTime_, 0.0f, 1.0f);
		ApplyAnimationBlend(skeleton_, *previous_, previousTime_, *current_, animationTime_, weight);
	} else if (duration > 0.0f) {
		ApplyAnimation(skeleton_, *current_, animationTime_);
	}
	UpdateSkeleton(skeleton_);

	if (skinCluster_) {
		// スキニング: 頂点はパレットで変形するのでローカル行列は単位行列
		UpdateSkinCluster(*skinCluster_, skeleton_);
		object3d_->SetLocalMatrix(TuboEngine::Math::MakeIdentity4x4());
	} else {
		// ノードアニメーション: メッシュを持つノードの行列でモデル全体を動かす
		object3d_->SetLocalMatrix(skeleton_.joints[meshJointIndex_].skeletonSpaceMatrix);
	}

	object3d_->Update();
}

// 描画処理
void TuboEngine::Animator::Draw() { object3d_->Draw(); }

bool TuboEngine::Animator::PlayAnimation(const std::string& name, float blendTime) {
	auto it = animations_.find(name);
	if (it == animations_.end()) {
		return false;
	}
	if (current_ == &it->second) {
		return true; // 既に再生中
	}
	// 今の姿勢から混ぜ始める（アニメーションが無い状態からは即切り替え）
	if (blendTime > 0.0f && current_->duration > 0.0f) {
		previous_ = current_;
		previousTime_ = animationTime_;
		blendTime_ = blendTime;
		blendElapsed_ = 0.0f;
	} else {
		previous_ = nullptr;
	}
	current_ = &it->second;
	currentName_ = name;
	animationTime_ = speed_ >= 0.0f ? 0.0f : current_->duration;
	eventsJustStarted_ = true;
	finishedNotified_ = false;
	return true;
}

void TuboEngine::Animator::AddAnimation(const std::string& name, const Animation& animation) {
	// 再生中のものを上書きしてもポインタはそのまま有効（std::map の要素は移動しない）
	Animation& stored = animations_[name];
	stored = animation;
	stored.name = name;
}

void TuboEngine::Animator::AddAnimation(const std::string& name, const std::string& filePath, uint32_t animationIndex) {
	AddAnimation(name, TuboEngine::LoadAnimation(kModelDirectory, filePath, animationIndex));
}

const TuboEngine::Animation* TuboEngine::Animator::FindAnimation(const std::string& name) const {
	auto it = animations_.find(name);
	return it != animations_.end() ? &it->second : nullptr;
}

void TuboEngine::Animator::LoadAnimations(const std::string& filePath) {
	std::vector<Animation> animations = TuboEngine::LoadAnimations(kModelDirectory, filePath);
	for (size_t i = 0; i < animations.size(); ++i) {
		std::string name = animations[i].name.empty() ? "Animation" + std::to_string(i) : animations[i].name;
		AddAnimation(name, animations[i]);
	}
}

void TuboEngine::Animator::SetAnimation(const Animation& animation, bool resetTime) {
	std::string name = animation.name.empty() ? "Custom" : animation.name;
	AddAnimation(name, animation);
	current_ = &animations_[name];
	currentName_ = name;
	previous_ = nullptr;
	if (resetTime) {
		animationTime_ = speed_ >= 0.0f ? 0.0f : current_->duration;
		eventsJustStarted_ = true;
	}
	finishedNotified_ = false;
}

void TuboEngine::Animator::LoadAnimation(const std::string& filePath, uint32_t animationIndex) {
	SetAnimation(TuboEngine::LoadAnimation(kModelDirectory, filePath, animationIndex));
}

std::vector<std::string> TuboEngine::Animator::GetAnimationNames() const {
	std::vector<std::string> names;
	for (const auto& [name, animation] : animations_) {
		names.push_back(name);
	}
	return names;
}

// ImGui描画
void TuboEngine::Animator::DrawImGui(const char* windowName) {
#ifdef USE_IMGUI
	// トランスフォーム・ライトは Object3d の ImGui を使う
	object3d_->DrawImGui(windowName);

	// 同じウィンドウにアニメーションの項目を追加する
	ImGui::Begin(windowName);
	ImGui::Separator();
	ImGui::Text("Animation");
	ImGui::Text("Name: %s%s", currentName_.c_str(), previous_ ? " (blending)" : "");
	ImGui::Text("Mode: %s / Joints: %d", IsSkinned() ? "Skinning" : "Node", int(skeleton_.joints.size()));
	if (ImGui::Button(isPlaying_ ? "Pause" : "Play")) {
		isPlaying_ = !isPlaying_;
	}
	ImGui::SameLine();
	ImGui::Checkbox("Loop", &isLoop_);
	ImGui::DragFloat("Speed", &speed_, 0.01f, -4.0f, 4.0f);
	ImGui::SliderFloat("Time", &animationTime_, 0.0f, std::max(current_->duration, 0.0001f));
	// 登録済みアニメーションの切り替え
	for (const auto& [name, animation] : animations_) {
		if (ImGui::Button(name.c_str())) {
			PlayAnimation(name);
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();
	ImGui::End();
#endif // USE_IMGUI
}

void TuboEngine::Animator::AddEvent(const std::string& animationName, float time, std::function<void()> callback, const std::string& eventName) {
	events_[animationName].push_back(AnimationEvent{time, eventName, std::move(callback)});
}

void TuboEngine::Animator::ClearEvents(const std::string& animationName) {
	if (animationName.empty()) {
		events_.clear();
	} else {
		events_.erase(animationName);
	}
}

void TuboEngine::Animator::FireEvents(float from, float to, float rawTo) {
	if (events_.empty()) {
		return;
	}
	const float duration = current_->duration;
	const std::string name = currentName_;
	const bool includeBegin = eventsJustStarted_; // 再生開始直後は from ちょうどのイベントも呼ぶ
	eventsJustStarted_ = false;

	if (speed_ >= 0.0f) {
		if (isLoop_ && rawTo >= duration) {
			// 終端を越えて先頭に戻った：残り → 先頭からの分
			FireEventsInRange(from, duration, includeBegin);
			if (currentName_ == name) { // イベントの中で別のアニメに切り替えていたら続きは呼ばない
				FireEventsInRange(0.0f, to, true);
			}
		} else {
			FireEventsInRange(from, to, includeBegin);
		}
	} else {
		if (isLoop_ && rawTo < 0.0f) {
			// 逆再生で先頭を越えて終端に戻った
			FireEventsInRange(from, 0.0f, includeBegin);
			if (currentName_ == name) {
				FireEventsInRange(duration, to, true);
			}
		} else {
			FireEventsInRange(from, to, includeBegin);
		}
	}
}

void TuboEngine::Animator::FireEventsInRange(float begin, float end, bool includeBegin) {
	auto it = events_.find(currentName_);
	if (it == events_.end()) {
		return;
	}
	const bool forward = end >= begin;
	// コールバックの中で AddEvent / PlayAnimation されても壊れないようコピーして回す
	const std::vector<AnimationEvent> events = it->second;
	const std::string animationName = currentName_;
	for (const AnimationEvent& e : events) {
		// 順再生は (begin, end]、逆再生は [end, begin)。begin ちょうどは前のフレームで呼んだので含めない
		// （再生開始直後だけは begin も含める＝0秒のイベントも呼ぶ）
		const bool afterBegin = forward ? (includeBegin ? e.time >= begin : e.time > begin) : (includeBegin ? e.time <= begin : e.time < begin);
		const bool beforeEnd = forward ? e.time <= end : e.time >= end;
		if (!afterBegin || !beforeEnd) {
			continue;
		}
		if (e.callback) {
			e.callback();
		}
		if (eventListener_) {
			eventListener_(animationName, e.name);
		}
	}
}

TuboEngine::Math::Matrix4x4 TuboEngine::Animator::GetJointWorldMatrix(const std::string& jointName) const {
	// Object3d と同じ計算: ワールド行列 = (スケルトン空間の行列) × (キャラクターの SRT)
	const TuboEngine::Math::Matrix4x4 world = TuboEngine::Math::MakeAffineMatrix(object3d_->GetScale(), object3d_->GetRotation(), object3d_->GetPosition());
	auto it = skeleton_.jointMap.find(jointName);
	if (it == skeleton_.jointMap.end()) {
		return world;
	}
	return skeleton_.joints[it->second].skeletonSpaceMatrix * world;
}

TuboEngine::Math::Vector3 TuboEngine::Animator::GetJointWorldPosition(const std::string& jointName) const {
	const TuboEngine::Math::Matrix4x4 m = GetJointWorldMatrix(jointName);
	return {m.m[3][0], m.m[3][1], m.m[3][2]};
}
