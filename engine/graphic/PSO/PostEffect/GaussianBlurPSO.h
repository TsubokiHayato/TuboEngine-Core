#pragma once
#include "PostEffectPSOBase.h"
/// <summary>
/// ガウシアンブラー用のPSOを生成・保持するクラス。
/// </summary>
class GaussianBlurPSO : public PostEffectPSOBase
{
public:
    // 初期化
    void Initialize() override;

    // グラフィックスパイプラインの作成
    void CreateGraphicPipeline();

    /// <summary>
    /// ルートシグネチャの生成。
    /// </summary>
    void CreateRootSignature() override;
};


