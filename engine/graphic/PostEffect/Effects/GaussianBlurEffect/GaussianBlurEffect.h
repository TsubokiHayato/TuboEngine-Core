#pragma once
#include"PostEffectBase.h"
#include"GaussianBlurPSO.h"
#include"DirectXCommon.h"
struct GaussianParams
{
    float sigma;
    float pad[3]; // 16-byte alignment
};

class GaussianBlurEffect : public PostEffectBase
{
public:
	GaussianBlurEffect();
	~GaussianBlurEffect();

    void Initialize() override;
    void Update() override;
    void DrawImGui() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;

    // ImGui等でパラメータを外部から変更したい場合
    GaussianParams* GetParams() { return params_; }

private:
    std::unique_ptr<GaussianBlurPSO> pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    GaussianParams* params_ = nullptr;
};
