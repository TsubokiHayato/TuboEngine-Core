#pragma once
#include "PSO/PostEffect/PostEffectPSOBase.h"

class VHSPSO : public PostEffectPSOBase
{
public:
    void Initialize() override;
    void CreateGraphicPipeline();
private:
    void CreateRootSignature() override;
};
