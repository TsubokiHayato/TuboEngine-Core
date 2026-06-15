#include "SkyBoxCommon.h"
#include "SkyBox/SkyBoxPSO.h"

namespace TuboEngine {

SkyBoxCommon* SkyBoxCommon::instance = nullptr; // シングルトンインスタンス
void SkyBoxCommon::Initialize() {

	/*---------------------------------------
	    PSOの初期化
	---------------------------------------*/
	pso = std::make_unique<SkyBoxPSO>();
	pso->Initialize();
}

void SkyBoxCommon::Finalize() {
	delete instance;
	instance = nullptr;
}

void SkyBoxCommon::DrawSettingsCommon() { pso->DrawSettingsCommon(); }

} // namespace TuboEngine
