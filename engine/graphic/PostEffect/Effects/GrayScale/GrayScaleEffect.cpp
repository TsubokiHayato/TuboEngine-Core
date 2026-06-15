#include "GrayScaleEffect.h"

void GrayScaleEffect::Initialize() {
    pso_ = std::make_unique<GrayScalePSO>();
    pso_->Initialize();
}

void GrayScaleEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    pso_->DrawSettingsCommon();
    // SRV等のセットはマネージャ側で
}
