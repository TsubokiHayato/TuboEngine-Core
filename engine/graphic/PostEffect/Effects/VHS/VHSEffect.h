#pragma once
#include "PostEffectBase.h"
#include "PSO/PostEffect/VHSPSO.h"
#include "DirectXCommon.h"

struct VHSParams
{
    float time;
    float intensity;
    float scanlineIntensity;
    float chromaticAberration;
};

class VHSEffect : public PostEffectBase
{
public:
    VHSEffect();
    ~VHSEffect();
    void Initialize() override;
    void Update() override;
    void DrawImGui() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;

private:
    std::unique_ptr<VHSPSO> pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    VHSParams* params_ = nullptr;
    float time_ = 0.0f;
};
