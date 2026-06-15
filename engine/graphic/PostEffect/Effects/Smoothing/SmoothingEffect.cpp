#include "SmoothingEffect.h"

void SmoothingEffect::Initialize() {

	pso_ = std::make_unique<SmoothingPSO>();
	pso_->Initialize();
}

void SmoothingEffect::Draw(ID3D12GraphicsCommandList* commandList) {
	pso_->DrawSettingsCommon();
	// SRV等のセットはマネージャ側で
}