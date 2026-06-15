#pragma once
#include "PostEffectBase.h"
#include "VignettePSO.h"
#include <wrl.h>
#include <memory>
#include "DirectXCommon.h"

// Vignette用パラメータ
struct VignetteParams
{
    float vignetteScale;
    float vignettePower;
    float pad[2]; // 16バイトアライメント
};

class VignetteEffect : public PostEffectBase
{
public:
    VignetteEffect();
    ~VignetteEffect();

    void Initialize() override;
    void Update() override;
    void DrawImGui() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;

    // ImGui等でパラメータを外部から変更したい場合
    VignetteParams* GetParams() { return params_; }

private:
    std::unique_ptr<VignettePSO> pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    VignetteParams* params_ = nullptr;
};

