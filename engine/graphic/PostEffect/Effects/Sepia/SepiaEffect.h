#pragma once
#include "PostEffectBase.h"
#include "SepiaPSO.h"
#include "DirectXCommon.h"

class SepiaEffect : public PostEffectBase
{
public:
    void Initialize() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;
private:
    std::unique_ptr<SepiaPSO> pso_;
};
