#pragma once
#include "PostEffectBase.h"
#include "SepiaPSO.h"
#include "DirectXCommon.h"

/// <summary>
/// 画面をセピア調にするポストエフェクト。
/// </summary>
class SepiaEffect : public PostEffectBase
{
public:
    /// <summary>
    /// 初期化処理。
    /// </summary>
    void Initialize() override;
    /// <summary>
    /// 描画処理。
    /// </summary>
    void Draw(ID3D12GraphicsCommandList* commandList) override;
private:
    std::unique_ptr<SepiaPSO> pso_;
};
