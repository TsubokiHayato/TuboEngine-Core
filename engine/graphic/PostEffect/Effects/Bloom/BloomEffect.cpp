#include "BloomEffect.h"
#include"ImGuiManager.h"

void BloomEffect::Initialize() {
	// PSO初期化
	pso_ = std::make_unique<BloomPSO>();
	pso_->Initialize();

	// 定数バッファ作成
	cbResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(BloomParams));
	cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&params_));
	// デフォルト値
	params_->threshold = 1.0f; // 明るさの閾値
	params_->intensity = 1.0f; // Bloomの強さ


}

void BloomEffect::Update() {}

void BloomEffect::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Bloom Effect Settings");
	ImGui::SliderFloat("Threshold", &params_->threshold, 0.0f, 5.0f, "%.2f");
	ImGui::SliderFloat("Intensity", &params_->intensity, 0.0f, 5.0f, "%.2f");
	ImGui::End();

#endif // USE_IMGUI
}

void BloomEffect::Draw(ID3D12GraphicsCommandList* commandList) {
	// PSO・ルートシグネチャ設定
	pso_->DrawSettingsCommon();
	// SRVやCBVのバインドはマネージャ側で行う場合は不要
	// ここでCBVをバインドする場合:
	commandList->SetGraphicsRootConstantBufferView(1, cbResource_->GetGPUVirtualAddress());



}