#pragma once
#include "PostEffectPSOBase.h"

class FlickerGlowPSO : public PostEffectPSOBase
{
public:
    void Initialize() override;
    void CreateGraphicPipeline();
    void CreateRootSignature() override;
};
