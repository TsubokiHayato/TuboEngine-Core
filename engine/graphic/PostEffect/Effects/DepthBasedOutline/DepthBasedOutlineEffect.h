#pragma once
#include "Camera.h"
#include "DepthBasedOutlinePSO.h"
#include "DirectXCommon.h"
#include "Matrix.h"
#include "PostEffectBase.h"

/// <summary>
/// 深度ベース輪郭線用の定数バッファデータ（射影行列）。
/// </summary>
struct ToonDepthOutlineParams {
	TuboEngine::Math::Matrix4x4 projectionInverse;
	TuboEngine::Math::Vector4 outlineColor; // アウトラインの色
	float outlineThickness; // アウトラインの太さ
	float outlineDepthThreshold; // アウトラインの深度しきい値
};

/// <summary>
/// 深度値の差を利用して輪郭線を描くポストエフェクト。
/// </summary>
class DepthBasedOutlineEffect : public PostEffectBase {
public:
	/// <summary>
	/// 初期化処理。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// 描画処理。
	/// </summary>
	void Draw(ID3D12GraphicsCommandList* commandList) override;
	/// <summary>
	/// 更新処理。
	/// </summary>
	void Update() override;
	/// <summary>
	/// ImGuiによるデバッグ表示。
	/// </summary>
	void DrawImGui() override;

public:
	/// <summary>
	/// メインカメラを設定する。
	/// </summary>
	void SetMainCamera(TuboEngine::Camera* camera) override;
	// 定数バッファの取得
	ID3D12Resource* GetMaterialCB() const { return materialCB_.Get(); }

private:
	TuboEngine::Camera* camera_ = nullptr; // メインカメラへのポインタ
	std::unique_ptr<DepthBasedOutlinePSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialCB_; // 定数バッファ
	ToonDepthOutlineParams* materialCBData_ = nullptr;
};
