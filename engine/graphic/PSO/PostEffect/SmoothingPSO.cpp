#include "SmoothingPSO.h"

void SmoothingPSO::Initialize() {
	PostEffectPSOBase::Initialize();
	CreateGraphicPipeline();
}

void SmoothingPSO::CreateGraphicPipeline() {
	// 必要ならルートパラメータ拡張（今回はベースのまま）
	// シェーダーパスを指定してベースのCreateGraphicPipelineを呼ぶ
	PostEffectPSOBase::CreateGraphicPipeline(
		L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
		L"Resources/Shaders/PostEffect/Smoothing.PS.hlsl"
	);
}