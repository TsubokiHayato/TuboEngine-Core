#pragma once
#include "BloomPSO.h"
#include "PostEffectBase.h"
#include"DirectXCommon.h"

/// <summary>
/// ブルームエフェクトの調整パラメータ。
/// </summary>
struct BloomParams {

	 float threshold; // 明るさの閾値
	float intensity; // Bloomの強さ
	float pad[2];

};

/// <summary>
/// 高輝度部分を抽出して光らせるブルームポストエフェクト。
/// </summary>
class BloomEffect : public PostEffectBase {
public:
	/// <summary>
	/// コンストラクタ。
	/// </summary>
	BloomEffect() = default;
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~BloomEffect() override = default;
	// 初期化
	void Initialize() override;
	// パラメータ更新（ImGuiやアニメーション用）
	void Update() override;
	// ImGui描画
	void DrawImGui() override;
	// 描画
	void Draw(ID3D12GraphicsCommandList* commandList) override;

private:
	std::unique_ptr<BloomPSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
	BloomParams* params_ = nullptr;
};
