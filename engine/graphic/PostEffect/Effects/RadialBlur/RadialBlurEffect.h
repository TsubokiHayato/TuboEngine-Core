#pragma once
#include "PostEffectBase.h"
#include "RadialBlurPSO.h"
#include"Vector2.h"
#include"DirectXCommon.h"

struct RadialBlurParams
{
	TuboEngine::Math::Vector2 radialBlurCenter; // 中心座標
	float radialBlurPower;  // 効果の強さ
	float pad[2]; // 16バイトアライメント

};

class RadialBlurEffect : public PostEffectBase
{
public:
	RadialBlurEffect();
	~RadialBlurEffect();
	void Initialize() override;
	void Update() override;
	void DrawImGui() override;
	void Draw(ID3D12GraphicsCommandList* commandList) override;

	// Dash等の演出用にパラメータを外部から操作
	void SetPower(float power);
	void SetCenter(const TuboEngine::Math::Vector2 & center);
	float GetPower() const;

private:
	std::unique_ptr<RadialBlurPSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
	RadialBlurParams* params_ = nullptr;

};

