#pragma once
#include "PostEffectPSOBase.h"

/// <summary>
/// SSAO（スクリーンスペース・アンビエントオクルージョン）用のPSOを生成・保持するクラス。
/// ルートシグネチャは DepthBasedOutline と同じ（t0:シーン, t1:深度, b0:パラメータ, s0/s1:サンプラ）。
/// </summary>
class SSAOPSO : public PostEffectPSOBase {
public:
	// 初期化
	void Initialize() override;

	// ルートシグネチャの作成
	void CreateRootSignature() override;

	// グラフィックスパイプラインの作成
	void CreateGraphicPipeline();
};
