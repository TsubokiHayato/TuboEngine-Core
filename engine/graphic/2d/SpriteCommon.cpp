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

	// param[10] (gCommonData, VS b1) 用の既定CBVを生成（useInstancing = 0）。
	// スプライト/テキストは Object3d.VS.hlsl を使うため param[10] を必ず初期化する必要がある。
	defaultCommonDataResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(int32_t) * 4); // 16byte アライン余裕
	int32_t* commonMapped = nullptr;
	defaultCommonDataResource_->Map(0, nullptr, reinterpret_cast<void**>(&commonMapped));
	*commonMapped = 0; // useInstancing = false
	defaultCommonDataResource_->Unmap(0, nullptr);
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

	// ルートシグネチャ束縛直後に param[10] (gCommonData) へ既定CBV(useInstancing=0)をセット。
	// スプライト/テキストの Draw は param 0/1/2 しか設定しないため、これが無いと
	// Object3d.VS.hlsl が読む param[10] が未初期化となり GBV が停止する。
	TuboEngine::DirectXCommon::GetInstance()->GetCommandList()
		->SetGraphicsRootConstantBufferView(10, defaultCommonDataResource_->GetGPUVirtualAddress());
}

} // namespace TuboEngine