#include "FlickerGlowEffect.h"
#include "ImGuiManager.h"
#include <chrono>

void FlickerGlowEffect::Initialize()
{
    pso_ = std::make_unique<FlickerGlowPSO>();
    pso_->Initialize();

    cbResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(FlickerGlowParams));
    cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&params_));

    params_->time = 0.0f;
    params_->intensity = 0.5f;      // 点灯速度
    params_->noiseAmount = 0.1f;    // ノイズ量
    params_->glowStrength = 0.3f;   // グロー寄与
}

void FlickerGlowEffect::Update()
{
    static auto startTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float t = std::chrono::duration<float>(now - startTime).count();
    params_->time = t;
}

void FlickerGlowEffect::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Begin("FlickerGlow Effect");
    ImGui::SliderFloat("Intensity", &params_->intensity, 0.1f, 3.0f);
    ImGui::SliderFloat("Noise", &params_->noiseAmount, 0.0f, 1.0f);
    ImGui::SliderFloat("Glow", &params_->glowStrength, 0.0f, 1.0f);
    ImGui::End();
#endif
}

void FlickerGlowEffect::Draw(ID3D12GraphicsCommandList* commandList)
{
    pso_->DrawSettingsCommon();
    commandList->SetGraphicsRootConstantBufferView(1, cbResource_->GetGPUVirtualAddress());
}
