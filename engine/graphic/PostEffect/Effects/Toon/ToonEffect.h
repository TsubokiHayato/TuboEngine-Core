#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "PostEffectBase.h"
#include "ToonPSO.h"
#include "Vector3.h"

// HLSL の cbuffer は 16バイト境界規則で float3 が境界をまたげない。
// C++ 側も同じ配置になるよう pad を挟んで合わせる（これを怠ると色がズレて読まれる）。
struct ToonParams {
	int stepCount = 3;                        // c0.x
	float toonRate;                           // c0.y
	float _pad0[2];                           // c0.zw
	TuboEngine::Math::Vector3 shadowColor;    // c1.xyz
	float _pad1;                              // c1.w
	TuboEngine::Math::Vector3 highlightColor; // c2.xyz
	float _pad2;                              // c2.w
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