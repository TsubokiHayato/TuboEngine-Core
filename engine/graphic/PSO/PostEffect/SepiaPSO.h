#pragma once
#include "PostEffectPSOBase.h"

/// <summary>
/// セピアエフェクト用のPSOを生成・保持するクラス。
/// </summary>
class SepiaPSO : public PostEffectPSOBase
{
public:
    // 初期化
    void Initialize() override;

    // グラフィックスパイプラインの作成
    void CreateGraphicPipeline();
};


