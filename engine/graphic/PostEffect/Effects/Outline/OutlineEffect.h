#include "PostEffectBase.h"
#include "OutlinePSO.h"
#include"DirectXCommon.h"


/// <summary>
/// 輝度差から輪郭線を描くポストエフェクト。
/// </summary>
class OutlineEffect : public PostEffectBase
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
    std::unique_ptr<OutlinePSO> pso_;
};
