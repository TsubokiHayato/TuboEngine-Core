#pragma once
#include "PostEffectBase.h"
#include"randomPSO.h"
#include <memory>
#include <wrl.h>
#include<chrono>

// randomEffect.h など
struct RandomParams
{
	float time;
};

class randomEffect : public PostEffectBase
{

public:

	randomEffect();
	~randomEffect();
	void Initialize() override;
	void Update() override;
	void DrawImGui() override;
	void Draw(ID3D12GraphicsCommandList* commandList) override;
	// ImGui等でパラメータを外部から変更したい場合
	RandomParams* GetParams() { return params_; }

private:
	std::unique_ptr<randomPSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
	RandomParams* params_ = nullptr;

	

};

