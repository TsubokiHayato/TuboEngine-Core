#include "ParticleCommon.h"

TuboEngine::ParticleCommon* TuboEngine::ParticleCommon::instance = nullptr; // シングルトンインスタンス
void TuboEngine::ParticleCommon::Initialize() {
	
	// PSOの初期化
	pso = std::make_unique<ParticlePSO>();
	pso->Initialize();

	

}
void TuboEngine::ParticleCommon::Finalize() {
	
	delete instance;
	instance = nullptr;
	
}
void TuboEngine::ParticleCommon::DrawSettingsCommon() {
	// PSOの共通描画設定
	pso->DrawSettingsCommon();

}
