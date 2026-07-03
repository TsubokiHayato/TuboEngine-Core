#pragma once
#include"PostEffectBase.h"
#include"SmoothingPSO.h"
#include"DirectXCommon.h"
/// <summary>
/// 画面を平滑化（ぼかし）するポストエフェクト。
/// </summary>
class SmoothingEffect : public PostEffectBase
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
    std::unique_ptr<SmoothingPSO> pso_;
};


