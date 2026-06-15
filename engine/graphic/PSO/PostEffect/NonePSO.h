#pragma once
#include "PostEffectPSOBase.h"

class NonePSO : public PostEffectPSOBase
{
public:
    // 初期化
    void Initialize() override;

    // グラフィックスパイプラインの作成
    void CreateGraphicPipeline();
};
