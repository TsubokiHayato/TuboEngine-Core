#include "NoneEffect.h"

void NoneEffect::Initialize() {
    pso_ = std::make_unique<NonePSO>();
    pso_->Initialize();
}

void NoneEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    pso_->DrawSettingsCommon();
    // SRV等のセットはマネージャ側で
}
