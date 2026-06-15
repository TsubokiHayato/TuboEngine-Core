#include "PostEffectBase.h"
#include "GrayScalePSO.h"
#include "DirectXCommon.h"


class GrayScaleEffect : public PostEffectBase
{
public:
    void Initialize() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;
private:
    std::unique_ptr<GrayScalePSO> pso_;
};
