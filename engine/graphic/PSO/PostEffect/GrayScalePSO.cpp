#include "GrayScalePSO.h"
#include "DirectXCommon.h"

void GrayScalePSO::Initialize() {
    PostEffectPSOBase::Initialize();
    CreateGraphicPipeline();
}

void GrayScalePSO::CreateGraphicPipeline() {
    // 必要ならルートパラメータ拡張（今回はベースのまま）
    // シェーダーパスを指定してベースのCreateGraphicPipelineを呼ぶ
    PostEffectPSOBase::CreateGraphicPipeline(
        L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
        L"Resources/Shaders/PostEffect/GrayScale.PS.hlsl"
    );
}
