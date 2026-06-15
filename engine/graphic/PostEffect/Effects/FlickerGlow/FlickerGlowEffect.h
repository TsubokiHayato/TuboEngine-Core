#pragma once
#include "PostEffectBase.h"
#include "FlickerGlowPSO.h"
#include "DirectXCommon.h"

struct FlickerGlowParams
{
    float time;
    float intensity;
    float noiseAmount;
    float glowStrength;
};

class FlickerGlowEffect : public PostEffectBase
{
public:
    void Initialize() override;
    void Update() override;
    void DrawImGui() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;

private:
    std::unique_ptr<FlickerGlowPSO> pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    FlickerGlowParams* params_ = nullptr;
};
