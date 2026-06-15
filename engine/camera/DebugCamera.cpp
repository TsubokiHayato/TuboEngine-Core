#include "DebugCamera.h"
#include "Input.h"
#include "Matrix.h"
#include "externals/imgui/imgui.h"
#include <algorithm>

namespace TuboEngine {

void DebugCamera::SetActive(bool active) {
	// OFF → ON になった瞬間に、現在のカメラ姿勢へスナップして視点の飛びを防ぐ
	if (active && !active_) {
		needSync_ = true;
	}
	active_ = active;
}

void DebugCamera::Update(Camera* target) {
	// F2 で ON/OFF 切り替え
	if (Input::GetInstance()->TriggerKey(DIK_F2)) {
		ToggleActive();
	}

	if (!active_ || !target) {
		return;
	}

	// 有効化直後は現在のカメラの姿勢を引き継ぐ
	if (needSync_) {
		translate_ = target->GetTranslate();
		rotate_ = target->GetRotation();
		needSync_ = false;
	}

	Input* input = Input::GetInstance();

	// 現在の姿勢から各方向ベクトル（前 / 右 / 上）を求める
	Math::Matrix4x4 rotMat = Math::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, rotate_, {0.0f, 0.0f, 0.0f});
	Math::Vector3 forward = Math::TransformCoord({0.0f, 0.0f, 1.0f}, rotMat);
	Math::Vector3 right   = Math::TransformCoord({1.0f, 0.0f, 0.0f}, rotMat);
	Math::Vector3 up      = Math::TransformCoord({0.0f, 1.0f, 0.0f}, rotMat);

	Input::MouseMove mm = input->GetMouseMove();

	// 右ドラッグ : 回転
	if (input->IsPressMouse(1)) {
		rotate_.y += mm.lX * rotateSpeed_; // ヨー
		rotate_.x += mm.lY * rotateSpeed_; // ピッチ
		// ピッチを約 ±89 度に制限して真上・真下での反転を防ぐ
		const float pitchLimit = 1.55f;
		rotate_.x = std::clamp(rotate_.x, -pitchLimit, pitchLimit);
	}

	// 中ドラッグ : パン（右・上方向に平行移動）
	if (input->IsPressMouse(2)) {
		translate_ += right * (-mm.lX * panSpeed_);
		translate_ += up * (mm.lY * panSpeed_);
	}

	// ホイール : 前後ドリー
	int32_t wheel = input->GetWheel();
	if (wheel != 0) {
		translate_ += forward * (wheel * wheelSpeed_);
	}

	// WASD : 前後左右 / Q,E : 上下
	if (input->PushKey(DIK_W)) { translate_ += forward * moveSpeed_; }
	if (input->PushKey(DIK_S)) { translate_ += forward * -moveSpeed_; }
	if (input->PushKey(DIK_D)) { translate_ += right * moveSpeed_; }
	if (input->PushKey(DIK_A)) { translate_ += right * -moveSpeed_; }
	if (input->PushKey(DIK_E)) { translate_ += up * moveSpeed_; }
	if (input->PushKey(DIK_Q)) { translate_ += up * -moveSpeed_; }

	// 対象カメラを乗っ取って姿勢を上書きする
	target->SetTranslate(translate_);
	target->setRotation(rotate_);
	target->setScale({1.0f, 1.0f, 1.0f});
	target->Update();
}

void DebugCamera::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("DebugCamera");
	bool active = active_;
	if (ImGui::Checkbox("Active (F2)", &active)) {
		SetActive(active);
	}
	ImGui::Text("RClick:Rotate  MClick:Pan  Wheel:Zoom");
	ImGui::Text("WASD:Move  Q/E:Up/Down");
	ImGui::Separator();
	ImGui::DragFloat3("Translate", &translate_.x, 0.1f);
	ImGui::DragFloat3("Rotate", &rotate_.x, 0.01f);
	ImGui::DragFloat("MoveSpeed", &moveSpeed_, 0.001f, 0.0f, 2.0f);
	ImGui::DragFloat("RotateSpeed", &rotateSpeed_, 0.0001f, 0.0f, 0.05f);
	ImGui::DragFloat("PanSpeed", &panSpeed_, 0.001f, 0.0f, 0.5f);
	ImGui::DragFloat("WheelSpeed", &wheelSpeed_, 0.001f, 0.0f, 0.5f);
	ImGui::End();
#endif // USE_IMGUI
}

} // namespace TuboEngine
