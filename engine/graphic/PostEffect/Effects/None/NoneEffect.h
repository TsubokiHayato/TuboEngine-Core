#include "PostEffectBase.h"
#include "NonePSO.h"
#include "DirectXCommon.h"


class NoneEffect : public PostEffectBase
{
public:
    void Initialize() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;
private:
    std::unique_ptr<NonePSO> pso_;
};
