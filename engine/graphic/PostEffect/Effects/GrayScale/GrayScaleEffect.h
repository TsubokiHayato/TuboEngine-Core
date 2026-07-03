#include "PostEffectBase.h"
#include "GrayScalePSO.h"
#include "DirectXCommon.h"


/// <summary>
/// 画面をグレースケール化するポストエフェクト。
/// </summary>
class GrayScaleEffect : public PostEffectBase
{
public:
    /// <summary>
    /// 初期化処理。
    /// </summary>
    void Initialize() override;
    /// <summary>
    /// 描画処理。
    /// </summary>
    void Draw(ID3D12GraphicsCommandList* commandList) override;
private:
    std::unique_ptr<GrayScalePSO> pso_;
};
