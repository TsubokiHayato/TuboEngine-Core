#include "PostEffectBase.h"
#include "NonePSO.h"
#include "DirectXCommon.h"


/// <summary>
/// 何も加工せずそのまま出力するポストエフェクト。
/// </summary>
class NoneEffect : public PostEffectBase
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
    std::unique_ptr<NonePSO> pso_;
};
