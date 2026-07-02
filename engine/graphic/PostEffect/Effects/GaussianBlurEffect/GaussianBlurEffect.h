#pragma once
#include"PostEffectBase.h"
#include"GaussianBlurPSO.h"
#include"DirectXCommon.h"
/// <summary>
/// ガウシアンブラーの調整パラメータ。
/// </summary>
struct GaussianParams
{
    float sigma;
    float pad[3]; // 16-byte alignment
};

/// <summary>
/// ガウシアンブラー（ぼかし）を掛けるポストエフェクト。
/// </summary>
class GaussianBlurEffect : public PostEffectBase
{
public:
	/// <summary>
	/// コンストラクタ。
	/// </summary>
	GaussianBlurEffect();
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~GaussianBlurEffect();

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
    GaussianParams* GetParams() { return params_; }

private:
    std::unique_ptr<GaussianBlurPSO> pso_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbResource_;
    GaussianParams* params_ = nullptr;
};
