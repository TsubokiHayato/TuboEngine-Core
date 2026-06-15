#include "SepiaPSO.h"

void SepiaPSO::Initialize() {

	PostEffectPSOBase::Initialize();
	CreateGraphicPipeline();
}

void SepiaPSO::CreateGraphicPipeline() {
	// 必要ならルートパラメータ拡張（今回はベースのまま）
	// シェーダーパスを指定してベースのCreateGraphicPipelineを呼ぶ
	PostEffectPSOBase::CreateGraphicPipeline(
		L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
		L"Resources/Shaders/PostEffect/Sepia.PS.hlsl"
	);
}