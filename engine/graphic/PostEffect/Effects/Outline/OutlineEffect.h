#include "PostEffectBase.h"
#include "OutlinePSO.h"
#include"DirectXCommon.h"


class OutlineEffect : public PostEffectBase
{
public:
    void Initialize() override;
    void Draw(ID3D12GraphicsCommandList* commandList) override;
private:
    std::unique_ptr<OutlinePSO> pso_;
};
