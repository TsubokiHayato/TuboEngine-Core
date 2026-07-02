#pragma once
#include "PostEffectBase.h"
#include"randomPSO.h"
#include <memory>
#include <wrl.h>
#include<chrono>

/// <summary>
/// ランダムノイズエフェクトの調整パラメータ。
/// </summary>
struct RandomParams
{
	float time;
};

/// <summary>
/// 画面にランダムノイズを乗せるポストエフェクト。
/// </summary>
class randomEffect : public PostEffectBase
{

public:

	/// <summary>
	/// コンストラクタ。
	/// </summary>
	randomEffect();
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~randomEffect();
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
	RandomParams* GetParams() { return params_; }

private:
	std::unique_ptr<randomPSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
	RandomParams* params_ = nullptr;

	

};

