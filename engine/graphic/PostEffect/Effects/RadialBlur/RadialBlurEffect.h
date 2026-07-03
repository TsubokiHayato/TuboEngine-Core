#pragma once
#include "PostEffectBase.h"
#include "RadialBlurPSO.h"
#include"Vector2.h"
#include"DirectXCommon.h"

/// <summary>
/// ラジアルブラーの調整パラメータ。
/// </summary>
struct RadialBlurParams
{
	TuboEngine::Math::Vector2 radialBlurCenter; // 中心座標
	float radialBlurPower;  // 効果の強さ
	float pad[2]; // 16バイトアライメント

};

/// <summary>
/// 画面中心から放射状にぼかすラジアルブラーポストエフェクト。
/// </summary>
class RadialBlurEffect : public PostEffectBase
{
public:
	/// <summary>
	/// コンストラクタ。
	/// </summary>
	RadialBlurEffect();
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~RadialBlurEffect();
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

	// Dash等の演出用にパラメータを外部から操作
	void SetPower(float power);
	void SetCenter(const TuboEngine::Math::Vector2 & center);
	float GetPower() const;

private:
	std::unique_ptr<RadialBlurPSO> pso_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
	RadialBlurParams* params_ = nullptr;

};

