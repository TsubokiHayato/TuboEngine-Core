#pragma once
#include"PostEffectPSOBase.h"

class DepthBasedOutlinePSO : public PostEffectPSOBase {
public:
	// 初期化
	void Initialize() override;

	// ルートシグネチャの作成
	void CreateRootSignature() override;

	// グラフィックスパイプラインの作成
	void CreateGraphicPipeline();
};
