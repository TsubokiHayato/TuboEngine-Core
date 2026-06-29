#pragma once
#include "DirectXCommon.h"
#include "PostEffectManager.h"
#include <d3d12.h>
#include <memory>
#include <wrl.h>
#include "Camera.h"
#include"Effects/DepthBasedOutline/DepthBasedOutlineEffect.h"

// 前方宣言
class WinApp;
class DirectXCommon;
class OffScreenRenderingPSO;
class GrayScalePSO;
class VignettePSO;

// オフスクリーンレンダリングを管理するクラス
// レンダーテクスチャへの描画やリソースバリアの制御などを担当します。
class OffScreenRendering {
public:
	/// <summary>
	/// シングルトンインスタンス取得
	/// </summary>
	static OffScreenRendering* GetInstance() {
		if (!instance) {
			instance = new OffScreenRendering();
		}
		return instance;
	}

private:
	// コンストラクタ・デストラクタ・コピー禁止
	static OffScreenRendering* instance;
	OffScreenRendering() = default;
	~OffScreenRendering() = default;
	OffScreenRendering(const OffScreenRendering&) = delete;
	OffScreenRendering& operator=(const OffScreenRendering&) = delete;

public:
	///------------------------------------------------------------------------
	///                             メンバ関数
	///------------------------------------------------------------------------

	/// <summary>
	/// 初期化処理
	/// 必要なリソースの生成や各種設定を行います。
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画前設定
	/// レンダーターゲットのセットやクリアなどを行います。
	/// </summary>
	void PreDraw();

	/// <summary>
	/// レンダーテクスチャをシェーダリソース用にバリア遷移します。
	/// </summary>
	void TransitionRenderTextureToShaderResource();

	/// <summary>
	/// レンダーテクスチャをレンダーターゲット用にバリア遷移します。
	/// </summary>
	void TransitionRenderTextureToRenderTarget();

	void TransitionDepthTo(D3D12_RESOURCE_STATES newState);

	// 任意リソースを現在状態(cur)から目標状態(next)へ遷移する（curは更新される）
	// ping-pongチェーンのバリア管理用
	void Transition(ID3D12Resource* res, D3D12_RESOURCE_STATES& cur, D3D12_RESOURCE_STATES next);

	/// <summary>
	/// 描画処理
	/// レンダーテクスチャへの描画を行います。
	/// </summary>
	void Draw();

	void DrawImGui();

	void Finalize();

	/// <summary>
	/// レンダーターゲットリソースの作成
	/// 指定されたサイズ・フォーマット・クリアカラーでリソースを生成します。
	/// </summary>
	/// <param name="device">D3D12デバイス</param>
	/// <param name="width">幅</param>
	/// <param name="height">高さ</param>
	/// <param name="format">フォーマット</param>
	/// <param name="clearColor">クリアカラー</param>
	/// <returns>作成されたリソース</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource>
	    CreateRenderTargetResource(Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height, DXGI_FORMAT format, const TuboEngine::Math::Vector4& clearColor);

public:
	void SetCamera(TuboEngine::Camera* camera) { this->camera_ = camera; }

	// SSFR 用: オフスクリーン RTV ハンドルを公開
	D3D12_CPU_DESCRIPTOR_HANDLE GetOffscreenRtvHandle() const { return offscreenRtvHandle; }

	// Dash演出用: 一時的にRadialBlurへ切り替えて強度をブーストする
	// enable=false にすると元のポストエフェクトへ戻す
	void SetDashPostEffectEnabled(bool enable);
	// Dash中のラジアルブラー強度(blurWidth相当)を設定
	void SetDashRadialBlurPower(float power);

	// HP演出用: Vignetteの強度を継続設定（現在値を保存して、Disableで復帰）
	void SetLowHpVignetteEnabled(bool enable);
	void SetLowHpVignettePower(float power);

	// VHSEffectを有効/無効にする（有効にすると画面全体がノイズっぽくなります）
	// enable=falseにすると元のポストエフェクトへ戻す
	void SetVHSEffect(bool enabled);

private:
	///-----------------------------------------------------------------------
	///                             メンバ変数
	///-----------------------------------------------------------------------
	TuboEngine::Camera* camera_ = nullptr; // カメラオブジェクトへのポインタ

	// D3D12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	// コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;

	// オフスクリーン用RTVヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> offscreenRtvDescriptorHeap;

	// オフスクリーン用RTVハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvHandle{};

	// レンダーターゲットのクリアカラー
	const TuboEngine::Math::Vector4 kRenderTargetClearValue = {0.0f, 0.0f, 0.0f, 1.0f}; // RGBの値。青っぽい色;

	// レンダーテクスチャリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResource_;

	///-----------------------------------------------------------------------
	///         ping-pong 用中間テクスチャ（最大4枚チェーン用）
	///-----------------------------------------------------------------------
	Microsoft::WRL::ComPtr<ID3D12Resource> ppTexture_[2];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> ppRtvHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE ppRtvHandle_[2]{};
	// 各中間テクスチャの現在状態（フレームをまたいで追跡）
	D3D12_RESOURCE_STATES ppState_[2] = {D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET};
	// DirectXCommon SRVヒープ内のスロット（0=シーン,1=深度/マスク は使用済みなので2,3）
	static constexpr uint32_t kPpSrvIndex_[2] = {2, 3};

	// レンダーテクスチャの現在の状態
	D3D12_RESOURCE_STATES renderTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// リソースバリア構造体
	D3D12_RESOURCE_BARRIER renderingBarrier{};
	D3D12_RESOURCE_BARRIER depthBarrier{};
	D3D12_RESOURCE_STATES depthResourceState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	///--------------------------------------------------------------------------
	///                             PSO
	///---------------------------------------------------------------------------

	// オフスクリーン用PSOクラス
	OffScreenRenderingPSO* offScreenRenderingPSO = nullptr;

	// ヴィネット用PSOクラス
	VignettePSO* vignettePSO = nullptr;
	///-----------------------------------------------------------------------
	///                             リソース
	///------------------------------------------------------------------------

	Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource;
	// VignetteParams* vignetteData = nullptr;

	///-----------------------------------------------------------------------
	///                             PostEffectManager
	///-----------------------------------------------------------------------

	PostEffectManager postEffectManager;

	// Dash用状態
	bool dashPostEffectEnabled_ = false;
	int32_t dashEffectIndex_ = -1;      // RadialBlur の index
	int32_t savedEffectIndex_ = 0;      // 復帰用

	// LowHP用状態
	bool lowHpVignetteEnabled_ = false;
	float savedVignettePower_ = 0.8f;

	//VHS
	bool vhsPostEffectEnabled_ = false;
	int32_t vhsEffectIndex_ = -1; // VHSEffect の index
	int32_t savedVhsEffectIndex_ = 0; // 復帰用
};
