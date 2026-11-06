#include "UniversalCamera.h"
#include "EditorObjects.h"
#include "ImGuiManager.h"
#include "JsonManager.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace TuboEngine {

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kDegToRad = kPi / 180.0f;

Math::Vector3 Lerp3(const Math::Vector3& a, const Math::Vector3& b, float t) { return a + (b - a) * t; }

UniversalCamera::View LerpView(const UniversalCamera::View& a, const UniversalCamera::View& b, float t) {
	return {Lerp3(a.eye, b.eye, t), Lerp3(a.lookAt, b.lookAt, t), a.fovDeg + (b.fovDeg - a.fovDeg) * t};
}

// フレームレートに関係なく同じ速さで近づく割合（speed が大きいほど速い）
float Damp(float speed, float deltaTime) { return 1.0f - std::exp(-speed * deltaTime); }

const char* const kModeKeys[] = {"Behind", "BehindDiagonal", "TopDown"};
} // namespace

UniversalCamera::UniversalCamera() {
	// 真後ろ（アーマード・コア風）: 頭越しに前を見る・肩越し・強めの追従・前を広く
	Preset& behind = GetPreset(ViewMode::Behind);
	behind.distance = 6.0f;
	behind.pitchDeg = 12.0f;
	behind.lookHeight = 2.2f;
	behind.shoulder = 0.7f;
	behind.lookAhead = 1.0f;
	behind.followSpeed = 14.0f;
	behind.fovDeg = 28.0f;
	// 後ろ斜め
	Preset& diagonal = GetPreset(ViewMode::BehindDiagonal);
	diagonal.distance = 7.0f;
	diagonal.pitchDeg = 25.0f;
	diagonal.yawOffsetDeg = 40.0f;
	diagonal.lookHeight = 1.2f;
	diagonal.followSpeed = 8.0f;
	diagonal.fovDeg = 26.0f;
	// 見下ろし: 高い所から。ターゲットが回ってもカメラは回らない
	Preset& top = GetPreset(ViewMode::TopDown);
	top.distance = 14.0f;
	top.pitchDeg = 70.0f;
	top.lookHeight = 0.5f;
	top.followYaw = false;
	top.followSpeed = 6.0f;
	top.fovDeg = 26.0f;
}

const char* UniversalCamera::ViewModeName(ViewMode mode) {
	switch (mode) {
	case ViewMode::Behind: return "真後ろ";
	case ViewMode::BehindDiagonal: return "後ろ斜め";
	case ViewMode::TopDown: return "見下ろし";
	default: return "?";
	}
}

// -----------------------------------------------------------------------------
// 追従対象
// -----------------------------------------------------------------------------
void UniversalCamera::AddTarget(const std::string& name, std::function<Math::Vector3()> getPosition, std::function<float()> getYaw) {
	if (Target* t = FindTarget(name)) {
		t->getPosition = std::move(getPosition);
		t->getYaw = std::move(getYaw);
		return;
	}
	Target t;
	t.name = name;
	t.getPosition = std::move(getPosition);
	t.getYaw = std::move(getYaw);
	if (t.getPosition) t.lastPosition = t.getPosition();
	if (t.getYaw) t.lastYaw = t.getYaw();
	targets_.push_back(std::move(t));
	if (targetName_.empty()) {
		targetName_ = name; // 最初に登録したものを追従
	}
}

void UniversalCamera::RemoveTarget(const std::string& name) {
	std::erase_if(targets_, [&](const Target& t) { return t.name == name; });
	if (lockOnName_ == name) lockOnName_.clear();
	// 追従中のターゲットを消した時は、最後の位置で止まる（targetName_ はそのまま。見つからなければ動かない）
}

std::vector<std::string> UniversalCamera::GetTargetNames() const {
	std::vector<std::string> names;
	for (const Target& t : targets_) names.push_back(t.name);
	return names;
}

UniversalCamera::Target* UniversalCamera::FindTarget(const std::string& name) {
	for (Target& t : targets_) {
		if (t.name == name) return &t;
	}
	return nullptr;
}

void UniversalCamera::SetTarget(const std::string& name, float duration, Easing::Type ease) {
	if (name == targetName_) return;
	targetName_ = name;
	if (lockOnName_ == name) lockOnName_.clear(); // 自分自身にはロックオンしない
	smoothedValid_ = false;                      // 新しいターゲットの理想の位置から始め、移り変わりはイージングに任せる
	StartBlend(duration, ease);
}

void UniversalCamera::SetViewMode(ViewMode mode, float duration, Easing::Type ease) {
	if (mode == viewMode_) return;
	viewMode_ = mode;
	smoothedValid_ = false;
	StartBlend(duration, ease);
}

// -----------------------------------------------------------------------------
// ロックオン / シェイク / FOV / キーフレーム / 手動
// -----------------------------------------------------------------------------
void UniversalCamera::LockOn(const std::string& name, float duration, Easing::Type ease) {
	if (name == lockOnName_ || name == targetName_ || !FindTarget(name)) return;
	lockOnName_ = name;
	smoothedValid_ = false;
	StartBlend(duration, ease);
}

void UniversalCamera::ClearLockOn(float duration, Easing::Type ease) {
	if (lockOnName_.empty()) return;
	lockOnName_.clear();
	smoothedValid_ = false;
	StartBlend(duration, ease);
}

void UniversalCamera::Shake(float strength) { trauma_ = std::clamp(trauma_ + strength, 0.0f, 1.0f); }

void UniversalCamera::KickFov(float addDeg, float duration, Easing::Type ease) {
	fovKick_ = addDeg;
	fovKickTime_ = 0.0f;
	fovKickDuration_ = (std::max)(duration, 0.0001f);
	fovKickEase_ = ease;
}

void UniversalCamera::PlayClip(const KeyframeClip* clip, float blendIn, float blendOut, Easing::Type ease) {
	if (!clip) return;
	clip_ = clip;
	clipPlayer_.SetClip(clip);
	clipPlayer_.Play();
	clipBlendOut_ = blendOut;
	clipEase_ = ease;
	manual_ = false;
	StartBlend(blendIn, ease);
}

void UniversalCamera::StopClip(float blendOut, Easing::Type ease) {
	if (!clip_) return;
	clip_ = nullptr;
	clipPlayer_.Stop();
	smoothedValid_ = false;
	StartBlend(blendOut, ease);
}

void UniversalCamera::SetManualView(const View& view) {
	manual_ = true;
	manualView_ = view;
	blendDuration_ = 0.0f; // 手動で決めた値はそのまま使う
}

void UniversalCamera::ReleaseManual(float duration, Easing::Type ease) {
	if (!manual_) return;
	manual_ = false;
	smoothedValid_ = false;
	StartBlend(duration, ease);
}

void UniversalCamera::StartBlend(float duration, Easing::Type ease) {
	blendFrom_ = current_; // 「今のカメラ」から移る（切り替えの途中でも跳ばない）
	blendTime_ = 0.0f;
	blendDuration_ = (std::max)(duration, 0.0f);
	blendEase_ = ease;
}

// -----------------------------------------------------------------------------
// 更新
// -----------------------------------------------------------------------------
UniversalCamera::View UniversalCamera::ComputeFollowView(float deltaTime) {
	Target* target = FindTarget(targetName_);
	if (!target) {
		return current_; // 追従先が無い → その場で止まる
	}
	if (target->getPosition) target->lastPosition = target->getPosition();
	if (target->getYaw) target->lastYaw = target->getYaw();
	const Math::Vector3 position = target->lastPosition;
	const Preset& p = presets_[static_cast<size_t>(viewMode_)];

	// 向き: ターゲットの向き（見下ろしは固定）。ロックオン中は「自分 → 敵」の方向
	float yaw = p.followYaw ? target->lastYaw : 0.0f;
	float extraDistance = 0.0f;
	Target* lock = lockOnName_.empty() ? nullptr : FindTarget(lockOnName_);
	if (lock) {
		if (lock->getPosition) lock->lastPosition = lock->getPosition();
		Math::Vector3 toLock = lock->lastPosition - position;
		toLock.y = 0.0f;
		const float length = toLock.Length();
		if (length > 0.001f) {
			yaw = std::atan2(toLock.x, toLock.z);
		}
		extraDistance = (std::min)(length * lockOnExtraDistance, lockOnMaxExtraDistance);
	}
	yaw += p.yawOffsetDeg * kDegToRad;

	// 向きはなめらかに追いかける（近い方向に回る）
	const float k = Damp(p.followSpeed, deltaTime);
	if (!smoothedValid_) {
		smoothedYaw_ = yaw;
	} else {
		smoothedYaw_ += std::remainder(yaw - smoothedYaw_, 2.0f * kPi) * k;
	}

	const float pitch = std::clamp(p.pitchDeg, -89.0f, 89.0f) * kDegToRad;
	const Math::Vector3 forward{std::sin(smoothedYaw_), 0.0f, std::cos(smoothedYaw_)};
	const Math::Vector3 right{std::cos(smoothedYaw_), 0.0f, -std::sin(smoothedYaw_)};
	const Math::Vector3 back{-std::sin(smoothedYaw_) * std::cos(pitch), std::sin(pitch), -std::cos(smoothedYaw_) * std::cos(pitch)};

	View ideal;
	ideal.lookAt = position + Math::Vector3{0.0f, p.lookHeight, 0.0f} + right * p.shoulder + forward * p.lookAhead;
	if (lock) {
		ideal.lookAt = Lerp3(ideal.lookAt, lock->lastPosition + Math::Vector3{0.0f, p.lookHeight, 0.0f}, lockOnLookWeight);
	}
	ideal.eye = ideal.lookAt + back * (p.distance + extraDistance);
	ideal.fovDeg = p.fovDeg;

	// 位置もなめらかに追いかける
	if (!smoothedValid_) {
		smoothed_ = ideal;
		smoothedValid_ = true;
	} else {
		smoothed_ = LerpView(smoothed_, ideal, k);
	}
	return smoothed_;
}

void UniversalCamera::Update(float deltaTime) {
	// 1. 目標の見え方（手動 > キーフレーム > 追従）
	View desired;
	if (manual_) {
		desired = manualView_;
	} else if (clip_) {
		clipPlayer_.Update(deltaTime);
		desired = current_;
		Math::Vector4 v;
		if (clipPlayer_.Evaluate("camera:eye", v)) desired.eye = {v.x, v.y, v.z};
		if (clipPlayer_.Evaluate("camera:lookAt", v)) desired.lookAt = {v.x, v.y, v.z};
		if (clipPlayer_.Evaluate("camera:fov", v)) desired.fovDeg = v.x;
		if (clipPlayer_.IsFinished()) {
			// 最後の見え方から追従へ戻る
			current_ = desired;
			StopClip(clipBlendOut_, clipEase_);
			desired = ComputeFollowView(deltaTime);
		}
	} else {
		desired = ComputeFollowView(deltaTime);
	}

	// 2. 切り替えのイージング（覚えておいた「切り替えた時の見え方」→ 毎フレームの目標）
	if (blendTime_ < blendDuration_) {
		blendTime_ += deltaTime;
		const float t = std::clamp(blendTime_ / blendDuration_, 0.0f, 1.0f);
		current_ = LerpView(blendFrom_, desired, Easing::Apply(blendEase_, t));
	} else {
		current_ = desired;
	}

	// 3. シェイク（強さは2乗で効かせると、弱い時は控えめ・強い時ははっきり揺れる）
	trauma_ = (std::max)(0.0f, trauma_ - shakeDecay * deltaTime);
	shakeTime_ += deltaTime;
	Math::Vector3 shake{};
	if (trauma_ > 0.0f) {
		const float amount = shakeMaxOffset * trauma_ * trauma_;
		const float t = shakeTime_ * shakeFrequency;
		shake = {(std::sin(t * 1.00f) * 0.6f + std::sin(t * 2.31f + 1.3f) * 0.4f) * amount,
		         (std::sin(t * 1.17f + 2.1f) * 0.6f + std::sin(t * 2.73f + 0.4f) * 0.4f) * amount,
		         (std::sin(t * 0.93f + 4.2f) * 0.6f + std::sin(t * 2.07f + 3.1f) * 0.4f) * amount * 0.5f};
	}

	// 4. FOV（上乗せはなめらかに、キックは一瞬広げて戻す）
	fovOffset_ += (fovOffsetTarget_ - fovOffset_) * Damp(8.0f, deltaTime);
	float kick = 0.0f;
	if (fovKickTime_ < fovKickDuration_) {
		fovKickTime_ += deltaTime;
		const float t = std::clamp(fovKickTime_ / fovKickDuration_, 0.0f, 1.0f);
		kick = fovKick_ * (1.0f - Easing::Apply(fovKickEase_, t));
	}

	ApplyToCamera(current_, fovOffset_ + kick, shake);
}

void UniversalCamera::ApplyToCamera(const View& view, float extraFovDeg, const Math::Vector3& shakeOffset) {
	const Math::Vector3 eye = view.eye + shakeOffset;
	const Math::Vector3 lookAt = view.lookAt + shakeOffset * 0.3f;
	Math::Vector3 forward = lookAt - eye;
	const float length = forward.Length();
	if (length > 0.0001f) {
		forward = forward / length;
	} else {
		forward = {0.0f, 0.0f, 1.0f};
	}
	// 回転は「eye → lookAt」の向きから決める（オイラー角どうしを補間しないので破綻しにくい）
	camera_.SetTranslate(eye);
	camera_.setRotation({std::asin(std::clamp(-forward.y, -1.0f, 1.0f)), std::atan2(forward.x, forward.z), 0.0f});
	camera_.setFovY(std::clamp(view.fovDeg + extraFovDeg, 1.0f, 170.0f) * kDegToRad);
	camera_.Update();
}

// -----------------------------------------------------------------------------
// 保存
// -----------------------------------------------------------------------------
bool UniversalCamera::LoadPresets(const std::string& path, bool logIfMissing) {
	JsonManager::Json root;
	if (!JsonManager::Load(path, root, logIfMissing) || !root.is_object()) return false;
	for (size_t i = 0; i < presets_.size(); ++i) {
		if (!root.contains(kModeKeys[i])) continue;
		const JsonManager::Json& j = root[kModeKeys[i]];
		Preset& p = presets_[i];
		JsonManager::Read(j, "distance", p.distance);
		JsonManager::Read(j, "pitchDeg", p.pitchDeg);
		JsonManager::Read(j, "yawOffsetDeg", p.yawOffsetDeg);
		JsonManager::Read(j, "lookHeight", p.lookHeight);
		JsonManager::Read(j, "shoulder", p.shoulder);
		JsonManager::Read(j, "lookAhead", p.lookAhead);
		JsonManager::Read(j, "followYaw", p.followYaw);
		JsonManager::Read(j, "followSpeed", p.followSpeed);
		JsonManager::Read(j, "fovDeg", p.fovDeg);
	}
	if (root.contains("lockOn")) {
		JsonManager::Read(root["lockOn"], "lookWeight", lockOnLookWeight);
		JsonManager::Read(root["lockOn"], "extraDistance", lockOnExtraDistance);
		JsonManager::Read(root["lockOn"], "maxExtraDistance", lockOnMaxExtraDistance);
	}
	if (root.contains("shake")) {
		JsonManager::Read(root["shake"], "maxOffset", shakeMaxOffset);
		JsonManager::Read(root["shake"], "frequency", shakeFrequency);
		JsonManager::Read(root["shake"], "decay", shakeDecay);
	}
	return true;
}

bool UniversalCamera::SavePresets(const std::string& path) const {
	JsonManager::Json root;
	for (size_t i = 0; i < presets_.size(); ++i) {
		const Preset& p = presets_[i];
		root[kModeKeys[i]] = {{"distance", p.distance}, {"pitchDeg", p.pitchDeg}, {"yawOffsetDeg", p.yawOffsetDeg}, {"lookHeight", p.lookHeight}, {"shoulder", p.shoulder},
		                      {"lookAhead", p.lookAhead}, {"followYaw", p.followYaw}, {"followSpeed", p.followSpeed}, {"fovDeg", p.fovDeg}};
	}
	root["lockOn"] = {{"lookWeight", lockOnLookWeight}, {"extraDistance", lockOnExtraDistance}, {"maxExtraDistance", lockOnMaxExtraDistance}};
	root["shake"] = {{"maxOffset", shakeMaxOffset}, {"frequency", shakeFrequency}, {"decay", shakeDecay}};
	return JsonManager::Save(path, root);
}

// -----------------------------------------------------------------------------
// ImGui
// -----------------------------------------------------------------------------
void UniversalCamera::DrawImGui(const char* windowName) {
#ifdef USE_IMGUI
	// Hierarchy / Inspector / Timeline に "Camera" として出す（Timeline でカメラのキーを打てる）
	{
		Editor::EditorObject e;
		e.name = "Camera";
		e.type = "Camera";
		auto setView = [this](auto&& modify) {
			View view = manual_ ? manualView_ : current_;
			modify(view);
			SetManualView(view);
		};
		e.extraProperties.push_back({"camera:eye", "カメラ位置", 3, [this] { return Math::Vector4{current_.eye.x, current_.eye.y, current_.eye.z, 0.0f}; },
		                             [setView](const Math::Vector4& v) { setView([&](View& view) { view.eye = {v.x, v.y, v.z}; }); }});
		e.extraProperties.push_back({"camera:lookAt", "注視点", 3, [this] { return Math::Vector4{current_.lookAt.x, current_.lookAt.y, current_.lookAt.z, 0.0f}; },
		                             [setView](const Math::Vector4& v) { setView([&](View& view) { view.lookAt = {v.x, v.y, v.z}; }); }});
		e.extraProperties.push_back({"camera:fov", "画角", 1, [this] { return Math::Vector4{current_.fovDeg, 0.0f, 0.0f, 0.0f}; },
		                             [setView](const Math::Vector4& v) { setView([&](View& view) { view.fovDeg = v.x; }); }});
		e.drawExtra = [this] {
			ImGui::SeparatorText("カメラ（手動）");
			View view = manual_ ? manualView_ : current_;
			bool changed = ImGui::DragFloat3("Eye", &view.eye.x, 0.05f);
			changed |= ImGui::DragFloat3("LookAt", &view.lookAt.x, 0.05f);
			changed |= ImGui::DragFloat("FOV (deg)", &view.fovDeg, 0.2f, 1.0f, 170.0f);
			if (changed) SetManualView(view);
			if (manual_ && ImGui::Button("追従に戻す")) ReleaseManual(uiDuration_, static_cast<Easing::Type>(uiEase_));
			ImGui::TextWrapped("値を変えると手動（追従を止める）になります。Timeline の「カメラ位置 / 注視点 / 画角」でキーを打てます。");
		};
		Editor::Register(std::move(e));
	}

	if (!ImGuiManager::GetInstance()->BeginPanel(windowName)) {
		ImGuiManager::GetInstance()->EndPanel();
		return;
	}
	const Easing::Type ease = static_cast<Easing::Type>(uiEase_);
	const char* state = manual_ ? "手動" : (clip_ ? "キーフレーム再生中" : "追従");
	ImGui::Text("状態: %s / 視点: %s", state, ViewModeName(viewMode_));
	if (manual_) {
		ImGui::SameLine();
		if (ImGui::SmallButton("追従に戻す")) ReleaseManual(uiDuration_, ease);
	}
	if (clip_) {
		ImGui::SameLine();
		if (ImGui::SmallButton("再生を止める")) StopClip(uiDuration_, ease);
	}

	// 切り替えの設定
	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragFloat("切り替え時間", &uiDuration_, 0.01f, 0.0f, 5.0f, "%.2f 秒");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(140.0f);
	ImGui::Combo("イージング", &uiEase_, Easing::Names(), static_cast<int>(Easing::Type::Count));

	// 追従対象
	const std::vector<std::string> names = GetTargetNames();
	if (ImGui::BeginCombo("追従対象", targetName_.empty() ? "(なし)" : targetName_.c_str())) {
		for (const std::string& name : names) {
			if (ImGui::Selectable(name.c_str(), name == targetName_)) SetTarget(name, uiDuration_, ease);
		}
		ImGui::EndCombo();
	}

	// 視点
	for (int i = 0; i < static_cast<int>(ViewMode::Count); ++i) {
		const ViewMode mode = static_cast<ViewMode>(i);
		if (i > 0) ImGui::SameLine();
		const bool active = mode == viewMode_;
		if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		if (ImGui::Button(ViewModeName(mode))) SetViewMode(mode, uiDuration_, ease);
		if (active) ImGui::PopStyleColor();
	}

	// ロックオン
	ImGui::SeparatorText("ロックオン");
	if (ImGui::BeginCombo("相手", lockOnName_.empty() ? "(解除)" : lockOnName_.c_str())) {
		if (ImGui::Selectable("(解除)", lockOnName_.empty())) ClearLockOn(uiDuration_, ease);
		for (const std::string& name : names) {
			if (name == targetName_) continue;
			if (ImGui::Selectable(name.c_str(), name == lockOnName_)) LockOn(name, uiDuration_, ease);
		}
		ImGui::EndCombo();
	}
	ImGui::DragFloat("注視点を相手へ寄せる", &lockOnLookWeight, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("距離に応じて引く", &lockOnExtraDistance, 0.01f, 0.0f, 2.0f);

	// シェイク
	ImGui::SeparatorText("シェイク");
	static float shakeStrength = 0.6f;
	ImGui::SetNextItemWidth(120.0f);
	ImGui::SliderFloat("強さ", &shakeStrength, 0.0f, 1.0f);
	ImGui::SameLine();
	if (ImGui::Button("揺らす")) Shake(shakeStrength);
	ImGui::SameLine();
	ImGui::ProgressBar(trauma_, ImVec2(80.0f, 0.0f), "");
	ImGui::DragFloat("最大のずれ", &shakeMaxOffset, 0.01f, 0.0f, 3.0f);
	ImGui::DragFloat("揺れの速さ", &shakeFrequency, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("収まる速さ", &shakeDecay, 0.05f, 0.0f, 10.0f);

	// FOV
	ImGui::SeparatorText("FOV");
	static float kickDeg = 8.0f;
	static float kickTime = 0.35f;
	ImGui::SetNextItemWidth(90.0f);
	ImGui::DragFloat("広げる (deg)", &kickDeg, 0.1f, -60.0f, 60.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90.0f);
	ImGui::DragFloat("戻る時間", &kickTime, 0.01f, 0.01f, 3.0f);
	ImGui::SameLine();
	if (ImGui::Button("キック")) KickFov(kickDeg, kickTime);
	ImGui::DragFloat("上乗せ (deg)", &fovOffsetTarget_, 0.1f, -60.0f, 60.0f);

	// 今の視点の設定
	ImGui::SeparatorText((std::string("視点の設定: ") + ViewModeName(viewMode_)).c_str());
	Preset& p = GetPreset(viewMode_);
	ImGui::DragFloat("距離", &p.distance, 0.05f, 0.1f, 100.0f);
	ImGui::DragFloat("見下ろす角度 (deg)", &p.pitchDeg, 0.5f, -89.0f, 89.0f);
	ImGui::DragFloat("横にずらす角度 (deg)", &p.yawOffsetDeg, 0.5f, -180.0f, 180.0f);
	ImGui::DragFloat("注視点の高さ", &p.lookHeight, 0.02f);
	ImGui::DragFloat("肩越し", &p.shoulder, 0.02f);
	ImGui::DragFloat("先読み", &p.lookAhead, 0.02f);
	ImGui::Checkbox("ターゲットの向きに合わせて回る", &p.followYaw);
	ImGui::DragFloat("追従の速さ", &p.followSpeed, 0.1f, 0.1f, 60.0f);
	ImGui::DragFloat("画角 (deg)", &p.fovDeg, 0.2f, 1.0f, 170.0f);

	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputText("##presetPath", uiPresetPath_, sizeof(uiPresetPath_));
	ImGui::SameLine();
	if (ImGui::Button("Save")) SavePresets(uiPresetPath_);
	ImGui::SameLine();
	if (ImGui::Button("Load")) LoadPresets(uiPresetPath_);
	ImGuiManager::GetInstance()->EndPanel();
#else
	(void)windowName;
#endif // USE_IMGUI
}

} // namespace TuboEngine
