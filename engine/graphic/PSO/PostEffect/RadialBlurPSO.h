#pragma once
#include "PostEffectPSOBase.h"
class RadialBlurPSO : public PostEffectPSOBase
{
public:
	void Initialize() override;
	void CreateGraphicPipeline();
	void CreateRootSignature() override;

};


