#include "SpriteCommon.h"

namespace TuboEngine {

SpriteCommon* SpriteCommon::instance = nullptr; // シングルトンインスタンス
void SpriteCommon::Initialize() {

	/*---------------------------------------
	      PSOの初期化
	  ---------------------------------------*/
	pso = std::make_unique<PSO>();
	pso->Initialize();

	// BlendPSOの初期化（初期はNormal）
	blendPso_ = std::make_unique<BlendPSO>();
	blendPso_->Initialize(kBlendModeNormal);
}

void SpriteCommon::Finalize() {

	delete instance;
	instance = nullptr;
}

void SpriteCommon::DrawSettingsCommon(int blendMode) {
	// PSOの共通描画設定
	switch (blendMode) {
	case 0:
		// NoneBlendPSO
		pso->DrawSettingsCommon();
		break;
	case 1:
		blendPso_->SetBlendMode(kBlendModeNormal);
		blendPso_->DrawSettingsCommon();
		break;
	case 2:
		blendPso_->SetBlendMode(kBlendModeAdd);
		blendPso_->DrawSettingsCommon();
		break;
	case 3:
		blendPso_->SetBlendMode(kBlendModeSubtract);
		blendPso_->DrawSettingsCommon();
		break;
	case 4:
		blendPso_->SetBlendMode(kBlendModeMultily);
		blendPso_->DrawSettingsCommon();
		break;
	case 5:
		blendPso_->SetBlendMode(kBlendModeScreen);
		blendPso_->DrawSettingsCommon();
		break;
	default:
		// デフォルトはNoneBlendPSO
		pso->DrawSettingsCommon();
		break;
	}
}

} // namespace TuboEngine