#pragma once
#include "PostEffectBase.h"
#include "VignettePSO.h"
#include <wrl.h>
#include <memory>
#include "DirectXCommon.h"

/// <summary>
/// ビネットエフェクトの調整パラメータ。
/// </summary>
struct VignetteParams
{
    float vignetteScale;
    float vignettePower;
    float pad[2]; // 16バイトアライメント
};

/// <summary>
/// 画面周辺を暗くするビネットポストエフェクト。
/// </summary>
class VignetteEffect : public PostEffectBase
{
public:
    /// <summary>
    /// コンストラクタ。
    /// </summary>
    VignetteEffect();
    /// <summary>
    /// デストラクタ。
    /// </summary>
    ~VignetteEffect();

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

    // ImGui等でパラメータを外部から変更したい場合
    VignetteParams* GetParams() { return params_; }

private:
    std::unique_ptr<VignettePSO> pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    VignetteParams* params_ = nullptr;
};

