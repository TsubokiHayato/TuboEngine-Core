#pragma once
#include "PostEffectPSOBase.h"

/// <summary>
/// 輪郭線エフェクト用のPSOを生成・保持するクラス。
/// </summary>
class OutlinePSO : public PostEffectPSOBase
{
public:
    // 初期化
    void Initialize() override;

    // グラフィックスパイプラインの作成
    void CreateGraphicPipeline();
};
