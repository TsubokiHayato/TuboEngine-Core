#include "RadialBlurEffect.h"
#include"ImGuiManager.h"  

RadialBlurEffect::RadialBlurEffect() = default;
RadialBlurEffect::~RadialBlurEffect() {
    if (cbResource_ && params_) {
        cbResource_->Unmap(0, nullptr);
        params_ = nullptr;
    }
}

void RadialBlurEffect::Initialize() {
    // PSO初期化
	pso_ = std::make_unique<RadialBlurPSO>();
	pso_->Initialize();

    // 定数バッファ作成
	cbResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(RadialBlurParams));
    cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&params_));
    // デフォルト値
	
	params_->radialBlurPower = 0.02f;
	params_->radialBlurCenter.x = 0.5f;
	params_->radialBlurCenter.y = 0.5f;
	params_->pad[0] = 0.0f;
	params_->pad[1] = 0.0f;
}

void RadialBlurEffect::Update() {}

void RadialBlurEffect::DrawImGui() {

#ifdef USE_IMGUI
    ImGui::Begin("Radial Effect");
	
	ImGui::DragFloat("Radial Blur Power", &params_->radialBlurPower, 0.01f,0.0f, 10.0f);
	ImGui::SliderFloat2("Radial Blur Center X", &params_->radialBlurCenter.x, 0.0f, 1.0f);
    ImGui::End();
#endif // USE_IMGUI
}

void RadialBlurEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    // PSO・ルートシグネチャ設定
    pso_->DrawSettingsCommon();
    // SRVやCBVのバインドはマネージャ側で行う場合は不要
    // ここでCBVをバインドする場合:
    commandList->SetGraphicsRootConstantBufferView(1, cbResource_->GetGPUVirtualAddress());
}

void RadialBlurEffect::SetPower(float power) {
	if (!params_) {
		return;
	}
	params_->radialBlurPower = power;
}

void RadialBlurEffect::SetCenter(const TuboEngine::Math::Vector2& center) {
	if (!params_) {
		return;
	}
	params_->radialBlurCenter = center;
}

float RadialBlurEffect::GetPower() const {
	return params_ ? params_->radialBlurPower : 0.0f;
}
