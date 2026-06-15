#include "GaussianBlurEffect.h"
#include"ImGuiManager.h"

GaussianBlurEffect::GaussianBlurEffect() = default;

GaussianBlurEffect::~GaussianBlurEffect() {
    if (cbResource_ && params_) {
        cbResource_->Unmap(0, nullptr);
        params_ = nullptr;
    }
}

void GaussianBlurEffect::Initialize() {
    // PSO初期化
    pso_ = std::make_unique<GaussianBlurPSO>();
    pso_->Initialize();

    // 定数バッファ作成
	cbResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(GaussianParams));
    cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&params_));
    // デフォルト値
	params_->sigma = 1.0f;
	params_->pad[0] = 0.0f;
	params_->pad[1] = 0.0f;
	params_->pad[2] = 0.0f;
}

void GaussianBlurEffect::Update() {}

void GaussianBlurEffect::DrawImGui() {

#ifdef USE_IMGUI
    ImGui::Begin("Vignette Effect");
    ImGui::SliderFloat("Vignette Scale", &params_->sigma, 0.0f, 32.0f);
    ImGui::End();
#endif // USE_IMGUI
}

void GaussianBlurEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    // PSO・ルートシグネチャ設定
    pso_->DrawSettingsCommon();
    // SRVやCBVのバインドはマネージャ側で行う場合は不要
    // ここでCBVをバインドする場合:
    commandList->SetGraphicsRootConstantBufferView(1, cbResource_->GetGPUVirtualAddress());
}
