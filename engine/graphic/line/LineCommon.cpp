#include "LineCommon.h"

///----------------------------------------------------
/// LineCommonの初期化処理
///----------------------------------------------------
void TuboEngine::LineCommon::Initialize() {
    // Line描画用パイプラインステートオブジェクト生成
    pso_ = std::make_unique<LinePSO>();
    // パイプラインステートオブジェクトの初期化
    pso_->Initialize();
}

///----------------------------------------------------
/// LineCommonの描画設定
///----------------------------------------------------
void TuboEngine::LineCommon::DrawSettingsCommon() {
    // LinePSOの描画設定を呼び出し
    pso_->DrawSettingsCommon();
}