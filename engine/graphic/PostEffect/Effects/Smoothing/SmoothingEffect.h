#pragma once
#include"PostEffectBase.h"
#include"SmoothingPSO.h"
#include"DirectXCommon.h"
class SmoothingEffect : public PostEffectBase
{
public:
    void Initialize() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;
private:
    std::unique_ptr<SmoothingPSO> pso_;
};


