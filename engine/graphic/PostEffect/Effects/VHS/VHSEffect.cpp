#include "VHSEffect.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"

VHSEffect::VHSEffect() {
    pso_ = std::make_unique<VHSPSO>();
}

VHSEffect::~VHSEffect() {
    if (cbResource_) {
        cbResource_->Unmap(0, nullptr);
    }
}

void VHSEffect::Initialize() {
    pso_->Initialize();

    // 定数バッファの作成
	TuboEngine::DirectXCommon* dxCommon = TuboEngine::DirectXCommon::GetInstance();
    cbResource_ = dxCommon->CreateBufferResource(sizeof(VHSParams));
    cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&params_));

    params_->time = 0.0f;
    params_->intensity = 1.0f;
    params_->scanlineIntensity = 1.0f;
    params_->chromaticAberration = 1.0f;
}

void VHSEffect::Update() {
    time_ += 1.0f / 60.0f;
    params_->time = time_;
}

void VHSEffect::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Begin("VHS Effect");
    ImGui::SliderFloat("Intensity", &params_->intensity, 0.0f, 2.0f);
    ImGui::SliderFloat("Scanline", &params_->scanlineIntensity, 0.0f, 2.0f);
    ImGui::SliderFloat("Chromatic Aberration", &params_->chromaticAberration, 0.0f, 5.0f);
    ImGui::End();
#endif
}

void VHSEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    pso_->DrawSettingsCommon();
    commandList->SetGraphicsRootConstantBufferView(1, cbResource_->GetGPUVirtualAddress());
}
