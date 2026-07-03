#pragma once
#include "PSO/PostEffect/PostEffectPSOBase.h"

/// <summary>
/// VHS風エフェクト用のPSOを生成・保持するクラス。
/// </summary>
class VHSPSO : public PostEffectPSOBase
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
private:
    /// <summary>
    /// ルートシグネチャの生成。
    /// </summary>
    void CreateRootSignature() override;
};
