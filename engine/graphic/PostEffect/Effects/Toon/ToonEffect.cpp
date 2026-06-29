#include "ToonEffect.h"
#include "ImGuiManager.h"
#include "Matrix.h"

ToonEffect::ToonEffect() = default;
ToonEffect::~ToonEffect() {
	if (toonCB_ && toonParams_) {
		toonCB_->Unmap(0, nullptr);
		toonParams_ = nullptr;
	}
}
void ToonEffect::Initialize() {
	pso_ = std::make_unique<ToonPSO>();
	pso_->Initialize();

	// 定数バッファ作成
	toonCB_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource((sizeof(ToonParams) + 255) & ~255);
	toonCB_->Map(0, nullptr, reinterpret_cast<void**>(&toonParams_));
	// デフォルト値
	toonParams_->stepCount = 3;
	toonParams_->toonRate = 0.5f; // トゥーンレートの初期値
	toonParams_->shadowColor = TuboEngine::Math::Vector3(0.0f, 0.0f, 0.0f);    // 影色の初期値（黒）
	toonParams_->highlightColor = TuboEngine::Math::Vector3(1.0f, 1.0f, 1.0f); // ハイライト色の初期値（白）
	// ※ このToonは輝度ベース。Toon.PS.hlsl は深度テクスチャ(t1)を使わないため、
	//   以前あった深度SRVの作成(slot1)は削除した（Dissolveのマスクと衝突する原因だった）。
}

void ToonEffect::Update() {
	
}

void ToonEffect::DrawImGui() {

#ifdef USE_IMGUI
	if (TuboEngine::ImGuiManager::GetInstance()->BeginPanel("Toon")) {
	ImGui::SliderInt("Step Count", &toonParams_->stepCount,1, 20);
	ImGui::DragFloat("Toon Rate", &toonParams_->toonRate, 0.01f, 0.0f, 1.0f, "%.2f");
	ImGui::ColorEdit3("Shadow Color", &toonParams_->shadowColor.x);
	ImGui::ColorEdit3("Highlight Color", &toonParams_->highlightColor.x);
	}
	TuboEngine::ImGuiManager::GetInstance()->EndPanel();
#endif // USE_IMGUI
}
void ToonEffect::Draw(ID3D12GraphicsCommandList* commandList) {
	pso_->DrawSettingsCommon();
	// 入力テクスチャ(t0)のバインドはマネージャ側で。ここではCBVのみ。
	commandList->SetGraphicsRootConstantBufferView(1, toonCB_->GetGPUVirtualAddress());
}

void ToonEffect::SetMainCamera(TuboEngine::Camera* camera) {

// カメラの設定を行う
	camera_ = camera;
}
