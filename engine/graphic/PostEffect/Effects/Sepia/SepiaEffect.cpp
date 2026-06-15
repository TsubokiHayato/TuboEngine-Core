#include "SepiaEffect.h"


void SepiaEffect::Initialize() {
    pso_ = std::make_unique<SepiaPSO>();
    pso_->Initialize();
}

void SepiaEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    pso_->DrawSettingsCommon();
    // SRV等のバインドはマネージャ側で
}
