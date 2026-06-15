#pragma once
#include "PostEffectPSOBase.h"
class ToonPSO : public PostEffectPSOBase {
public:
	// 初期化
	void Initialize() override;

	// 描画設定
	void CreateRootSignature() override;

	// グラフィックスパイプラインの作成
	void CreateGraphicPipeline();
};
