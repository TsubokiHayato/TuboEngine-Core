#pragma once
#include "PostEffectPSOBase.h"

/// <summary>
/// フリッカーグローエフェクト用のPSOを生成・保持するクラス。
/// </summary>
class FlickerGlowPSO : public PostEffectPSOBase
{
public:
    /// <summary>
    /// 初期化処理。
    /// </summary>
    void Initialize() override;
    /// <summary>
    /// グラフィックスパイプラインステートの生成。
    /// </summary>
    void CreateGraphicPipeline();
    /// <summary>
    /// ルートシグネチャの生成。
    /// </summary>
    void CreateRootSignature() override;
};
