#include "Object3dCommon.h"
TuboEngine::Object3dCommon* TuboEngine::Object3dCommon::instance = nullptr; // シングルトンインスタンス
void TuboEngine::Object3dCommon::Initialize() {
	// 引数がnullptrでないことを確認

	
	/*---------------------------------------
		PSOの初期化
	---------------------------------------*/
	pso = std::make_unique<PSO>();
	pso->Initialize();

	blendPso_ = std::make_unique<BlendPSO>();
	blendPso_->Initialize(kBlendModeNormal);

	// param[10] (gCommonData, VS b1) 用の既定CBVを生成（useInstancing = 0）。
	// Object3d.VS.hlsl は useInstancing を必ず読むため、どの描画でも param[10]
	// は初期化済みである必要がある。DrawSettingsCommon でこれをバインドする。
	defaultCommonDataResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(int32_t) * 4); // 256/16byte アライン余裕
	int32_t* commonMapped = nullptr;
	defaultCommonDataResource_->Map(0, nullptr, reinterpret_cast<void**>(&commonMapped));
	*commonMapped = 0; // useInstancing = false
	defaultCommonDataResource_->Unmap(0, nullptr);
}

void TuboEngine::Object3dCommon::Finalize() {

	delete instance;
	instance = nullptr;

}

void TuboEngine::Object3dCommon::DrawSettingsCommon(int blendMode) {
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
		// デフォルトはpso
		pso->DrawSettingsCommon();
		break;
	}

	// ルートシグネチャ束縛直後に param[10] (gCommonData) へ既定CBVをセット。
	// 個別の描画が param[10] を設定しなくても、VS が必ず初期化済みの値を読めるため
	// GBV の GPU_BASED_VALIDATION_ROOT_ARGUMENT_UNINITIALIZED を防げる。
	// 個別描画（Model::Draw / DrawInstanced 等）が後から param[10] を上書きするのは問題ない。
	TuboEngine::DirectXCommon::GetInstance()->GetCommandList()
		->SetGraphicsRootConstantBufferView(10, defaultCommonDataResource_->GetGPUVirtualAddress());

}
