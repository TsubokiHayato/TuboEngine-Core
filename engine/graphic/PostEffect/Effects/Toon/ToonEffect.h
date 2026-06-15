#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "PostEffectBase.h"
#include "ToonPSO.h"
#include "Vector3.h"

struct ToonParams {
	int stepCount = 3;
	float toonRate;
	TuboEngine::Math::Vector3 shadowColor;
	TuboEngine::Math::Vector3 highlightColor;
	float padding[2]; // HLSLと同じサイズになるように
};

class ToonEffect : public PostEffectBase {

public:
	ToonEffect();
	~ToonEffect();

	void Initialize() override;
	void Update() override;
	void DrawImGui() override;
	void Draw(ID3D12GraphicsCommandList* commandList) override;
	ToonParams* GetParams() { return toonParams_; }

public:
	void SetMainCamera(TuboEngine::Camera* camera) override;

private:
	TuboEngine::Camera* camera_ = nullptr; // メインカメラへのポインタ
	std::unique_ptr<ToonPSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> toonCB_;
	ToonParams* toonParams_ = nullptr;
};