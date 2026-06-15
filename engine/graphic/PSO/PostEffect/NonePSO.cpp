#include "NonePSO.h"
#include "DirectXCommon.h"

void NonePSO::Initialize() {
    PostEffectPSOBase::Initialize();
    CreateGraphicPipeline();
}

void NonePSO::CreateGraphicPipeline() {
    // 必要ならルートパラメータ拡張（今回はベースのまま）
    // シェーダーパスを指定してベースのCreateGraphicPipelineを呼ぶ
    PostEffectPSOBase::CreateGraphicPipeline(
        L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
        L"Resources/Shaders/PostEffect/CopyImage.PS.hlsl"
    );
}
