#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include "Matrix.h"
#include "PostEffectBase.h"
#include "SSAOPSO.h"

/// <summary>
/// SSAO 用の定数バッファデータ。HLSL の cbuffer gParams と並びを一致させること。
/// </summary>
struct SSAOParams {
	TuboEngine::Math::Matrix4x4 projectionInverse; // 深度→ビュー空間の逆射影行列
	float radius;      // サンプル半径（スクリーンピクセル）
	float bias;        // 自己遮蔽防止しきい値（ビュー空間距離）
	float intensity;   // AO の強さ
	float power;       // コントラスト
	float depthRange;  // これ以上離れた深度差は無視
};

/// <summary>
/// 深度バッファから遮蔽（くぼみの陰）を求めて陰影を落とすポストエフェクト（SSAO）。
/// </summary>
class SSAOEffect : public PostEffectBase {
public:
	void Initialize() override;
	void Draw(ID3D12GraphicsCommandList* commandList) override;
	void Update() override;
	void DrawImGui() override;
	void SetMainCamera(TuboEngine::Camera* camera) override;

private:
	// このエフェクトが使う深度SRVのスロット。
	// slot0=シーン,1=Dissolve,2/3=ping-pong,4=DepthBasedOutlineの深度 と衝突しないよう 5 を使う。
	static constexpr uint32_t kDepthSrvIndex = 5;

	TuboEngine::Camera* camera_ = nullptr;
	std::unique_ptr<SSAOPSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialCB_;
	SSAOParams* materialCBData_ = nullptr;
};
