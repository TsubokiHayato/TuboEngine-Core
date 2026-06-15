#include "OutlineEffect.h"

void OutlineEffect::Initialize() {
    pso_ = std::make_unique<OutlinePSO>();
    pso_->Initialize();
}

void OutlineEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    pso_->DrawSettingsCommon();
    // SRV等のセットはマネージャ側で
}
