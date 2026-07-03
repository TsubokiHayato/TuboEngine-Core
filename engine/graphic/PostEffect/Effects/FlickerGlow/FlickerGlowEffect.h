#pragma once
#include "PostEffectBase.h"
#include "FlickerGlowPSO.h"
#include "DirectXCommon.h"

/// <summary>
/// フリッカーグローエフェクトの調整パラメータ。
/// </summary>
struct FlickerGlowParams
{
    float time;
    float intensity;
    float noiseAmount;
    float glowStrength;
};

/// <summary>
/// 画面に明滅する発光を加えるポストエフェクト。
/// </summary>
class FlickerGlowEffect : public PostEffectBase
{
public:
    /// <summary>
    /// 初期化処理。
    /// </summary>
    void Initialize() override;
    /// <summary>
    /// 更新処理。
    /// </summary>
    void Update() override;
    /// <summary>
    /// ImGuiによるデバッグ表示。
    /// </summary>
    void DrawImGui() override;
    /// <summary>
    /// 描画処理。
    /// </summary>
    void Draw(ID3D12GraphicsCommandList* commandList) override;

private:
    std::unique_ptr<FlickerGlowPSO> pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    FlickerGlowParams* params_ = nullptr;
};
