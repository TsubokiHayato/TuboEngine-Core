#pragma once
#include"PostEffectPSOBase.h"

/// <summary>
/// 深度ベース輪郭線エフェクト用のPSOを生成・保持するクラス。
/// </summary>
class DepthBasedOutlinePSO : public PostEffectPSOBase {
public:
	// 初期化
	void Initialize() override;

	// ルートシグネチャの作成
	void CreateRootSignature() override;

	// グラフィックスパイプラインの作成
	void CreateGraphicPipeline();
};
