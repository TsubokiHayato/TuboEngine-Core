#pragma once
#include "PostEffectBase.h"
#include "PSO/PostEffect/VHSPSO.h"
#include "DirectXCommon.h"

/// <summary>
/// VHS風エフェクトの調整パラメータ。
/// </summary>
struct VHSParams
{
    float time;
    float intensity;
    float scanlineIntensity;
    float chromaticAberration;
};

/// <summary>
/// VHSビデオ風のノイズ・歪みを加えるポストエフェクト。
/// </summary>
class VHSEffect : public PostEffectBase
{
public:
    /// <summary>
    /// コンストラクタ。
    /// </summary>
    VHSEffect();
    /// <summary>
    /// デストラクタ。
    /// </summary>
    ~VHSEffect();
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
    std::unique_ptr<VHSPSO> pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    VHSParams* params_ = nullptr;
    float time_ = 0.0f;
};
