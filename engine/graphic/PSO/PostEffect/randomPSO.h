#pragma once
#include "PostEffectPSOBase.h"
/// <summary>
/// ランダムノイズエフェクト用のPSOを生成・保持するクラス。
/// </summary>
class randomPSO : public PostEffectPSOBase
{
public:
	// 初期化
	void Initialize() override;
	// グラフィックスパイプラインの作成
	void CreateGraphicPipeline();
	// 描画設定
	void CreateRootSignature() override;

};


