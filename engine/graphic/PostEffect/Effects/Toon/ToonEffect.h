#pragma once
#include "Camera.h"
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "PostEffectBase.h"
#include "ToonPSO.h"
#include "Vector3.h"

/// <summary>
/// トゥーン調エフェクトの調整パラメータ。
/// </summary>
struct ToonParams {
	int stepCount = 3;                        // c0.x
	float toonRate;                           // c0.y
	float _pad0[2];                           // c0.zw
	TuboEngine::Math::Vector3 shadowColor;    // c1.xyz
	float _pad1;                              // c1.w
	TuboEngine::Math::Vector3 highlightColor; // c2.xyz
	float _pad2;                              // c2.w
};

/// <summary>
/// 画面をトゥーン調（階調化）にするポストエフェクト。
/// </summary>
class ToonEffect : public PostEffectBase {

public:
	/// <summary>
	/// コンストラクタ。
	/// </summary>
	ToonEffect();
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~ToonEffect();

	/// <summary>
	/// 初期化処理。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// 更新処理。
	/// </summary>
	void Update() override;
	/// <summary>
	/// ImGuiによるデバッグ表示。
	/// </summary>
	void DrawImGui() override;
	/// <summary>
	/// 描画処理。
	/// </summary>
	void Draw(ID3D12GraphicsCommandList* commandList) override;
	/// <summary>
	/// 調整パラメータを取得する。
	/// </summary>
	ToonParams* GetParams() { return toonParams_; }

public:
	/// <summary>
	/// メインカメラを設定する。
	/// </summary>
	void SetMainCamera(TuboEngine::Camera* camera) override;

private:
	TuboEngine::Camera* camera_ = nullptr; // メインカメラへのポインタ
	std::unique_ptr<ToonPSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> toonCB_;
	ToonParams* toonParams_ = nullptr;
};