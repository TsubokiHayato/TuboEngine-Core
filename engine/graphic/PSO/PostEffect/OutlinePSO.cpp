#include "OutlinePSO.h"
#include "DirectXCommon.h"

void OutlinePSO::Initialize() {
    PostEffectPSOBase::Initialize();
    CreateGraphicPipeline();
}

void OutlinePSO::CreateGraphicPipeline() {
    // 必要ならルートパラメータ拡張（今回はベースのまま）
    // シェーダーパスを指定してベースのCreateGraphicPipelineを呼ぶ
    PostEffectPSOBase::CreateGraphicPipeline(
        L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
        L"Resources/Shaders/PostEffect/Outline.PS.hlsl"
    );
}
