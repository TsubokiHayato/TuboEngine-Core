#pragma once
#include "PostEffectBase.h"
#include"DissolvePSO.h"
#include "Vector3.h"
#include"DirectXCommon.h"

/// <summary>
/// ディゾルブエフェクトの調整パラメータ。
/// </summary>
struct DissolveParams
{
	float dissolveThreshold; // 0.0～1.0で制御
	TuboEngine::Math::Vector3 edgeColor; // 追加: エッジの色
	float edgeStrength; // 追加: エッジの強さ
	float edgeWidth; // 追加: エッジの幅
};

/// <summary>
/// ノイズテクスチャを用いて画面を溶かすように消すディゾルブポストエフェクト。
/// </summary>
class DissolveEffect : public PostEffectBase
{
public:
	/// <summary>
	/// コンストラクタ。
	/// </summary>
	DissolveEffect();
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~DissolveEffect();
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
	// ImGui等でパラメータを外部から変更したい場合
	DissolveParams* GetParams() { return params_; }

public:

	/// <summary>
	/// DissolveThreshold を設定する。
	/// </summary>
	void SetDissolveThreshold(float threshold) {
		if (params_) {
			params_->dissolveThreshold = threshold;
		}
	}
	
	/// <summary>
	/// MaskTextureFileName を設定する。
	/// </summary>
	void SetMaskTextureFileName(std::string& fileName) {
		maskTextureFileName_ = fileName;
	}

private:
	std::unique_ptr<DissolvePSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
	DissolveParams* params_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> maskTextureResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> maskTextureUploadResource_;

	// マスクテクスチャのファイル名
	std::string maskTextureFileName_ = "Resources/Textures/noise0.png";
	std::string maskTextureFileName2_ = "Resources/Textures/noise1.png";
};

