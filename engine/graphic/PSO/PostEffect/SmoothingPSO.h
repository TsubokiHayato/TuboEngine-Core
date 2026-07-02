#pragma once
#include "PostEffectPSOBase.h"

/// <summary>
/// 平滑化エフェクト用のPSOを生成・保持するクラス。
/// </summary>
class SmoothingPSO : public PostEffectPSOBase
{
public:
    // 初期化
    void Initialize() override;

    // グラフィックスパイプラインの作成
    void CreateGraphicPipeline();
};

