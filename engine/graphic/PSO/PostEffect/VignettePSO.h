#pragma once
#include "PostEffectPSOBase.h"

/// <summary>
/// ビネットエフェクト用のPSOを生成・保持するクラス。
/// </summary>
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
