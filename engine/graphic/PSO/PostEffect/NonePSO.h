#pragma once
#include "PostEffectPSOBase.h"

/// <summary>
/// 無加工出力用のPSOを生成・保持するクラス。
/// </summary>
class NonePSO : public PostEffectPSOBase
{
public:
    // 初期化
    void Initialize() override;

    // グラフィックスパイプラインの作成
    void CreateGraphicPipeline();
};
