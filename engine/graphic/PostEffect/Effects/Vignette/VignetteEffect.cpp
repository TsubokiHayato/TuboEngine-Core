#include "VignetteEffect.h"
#include "DirectXCommon.h"
#include"ImGuiManager.h"

VignetteEffect::VignetteEffect() = default;
VignetteEffect::~VignetteEffect() {
    if (cbResource_ && params_) {
        cbResource_->Unmap(0, nullptr);
        params_ = nullptr;
    }
}

void VignetteEffect::Initialize() {
    // PSO初期化
    pso_ = std::make_unique<VignettePSO>();
    pso_->Initialize();

    // 定数バッファ作成
	cbResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VignetteParams));
    cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&params_));
    // デフォルト値
    params_->vignetteScale = 16.0f;
    params_->vignettePower = 0.8f;
    params_->pad[0] = 0.0f;
    params_->pad[1] = 0.0f;
}

void VignetteEffect::Update() {
}

void VignetteEffect::DrawImGui() {

#ifdef USE_IMGUI
	ImGui::Begin("Vignette Effect");
	ImGui::SliderFloat("Vignette Scale", &params_->vignetteScale, 0.0f, 32.0f);
	ImGui::SliderFloat("Vignette Power", &params_->vignettePower, 0.0f, 10.0f);
	ImGui::End();
#endif // USE_IMGUI
}

void VignetteEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    // PSO・ルートシグネチャ設定
    pso_->DrawSettingsCommon();
    // SRVやCBVのバインドはマネージャ側で行う場合は不要
    // ここでCBVをバインドする場合:
    commandList->SetGraphicsRootConstantBufferView(1, cbResource_->GetGPUVirtualAddress());
}