#pragma once
#include "PostEffectBase.h"
#include"DissolvePSO.h"
#include "Vector3.h"
#include"DirectXCommon.h"

struct DissolveParams
{
	float dissolveThreshold; // 0.0～1.0で制御
	TuboEngine::Math::Vector3 edgeColor; // 追加: エッジの色
	float edgeStrength; // 追加: エッジの強さ
	float edgeWidth; // 追加: エッジの幅
};

class DissolveEffect : public PostEffectBase
{
public:
	DissolveEffect();
	~DissolveEffect();
	void Initialize() override;
	void Update() override;
	void DrawImGui() override;
	void Draw(ID3D12GraphicsCommandList* commandList) override;
	// ImGui等でパラメータを外部から変更したい場合
	DissolveParams* GetParams() { return params_; }

public:

	void SetDissolveThreshold(float threshold) {
		if (params_) {
			params_->dissolveThreshold = threshold;
		}
	}
	
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

