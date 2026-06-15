#pragma once
#include "PostEffectPSOBase.h"

class VignettePSO : public PostEffectPSOBase
{
public:
    // 初期化
    void Initialize() override;

    // グラフィックスパイプラインの作成
    void CreateGraphicPipeline();
	// 描画設定
    void CreateRootSignature() override;
};
