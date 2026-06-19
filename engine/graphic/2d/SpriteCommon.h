#pragma once
#include<memory>
#include"Object/PSO.h"
#include"BlendPSO.h"
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>

namespace TuboEngine {
class SpriteCommon {
public:
	/// <summary>
	/// シングルトンインスタンス取得
	/// </summary>
	static SpriteCommon* GetInstance() {
		if (!instance) {
			instance = new SpriteCommon();
		}
		return instance;
	}

private:
	// コンストラクタ・デストラクタ・コピー禁止
	static SpriteCommon* instance; // シングルトンインスタンス
	SpriteCommon() = default;
	~SpriteCommon() = default;
	SpriteCommon(const SpriteCommon&) = delete;
	SpriteCommon& operator=(const SpriteCommon&) = delete;

public:
	/*------------------------------------------------------------
	        関数
	------------------------------------------------------------*/

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	void Finalize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	/// <param name="blendMode">ブレンドモード</param>
	/// <remarks>
	/// 0: NoneBlendPSO
	/// 1: NormalBlendPSO
	/// 2: AddBlendPSO
	/// 3: SubtractBlendPSO
	/// 4: MultiplyBlendPSO
	/// 5: ScreenBlendPSO
	/// </remarks>
	void DrawSettingsCommon(int blendMode);

private:
	std::unique_ptr<PSO> pso = nullptr;  // PSOのユニークポインタ
	std::unique_ptr<BlendPSO> blendPso_; // ブレンドPSOのユニークポインタ

	// スプライト/テキストは Object3d 用の PSO(Object3d.VS.hlsl) を間借りして描画している。
	// そのVSは param[10] (gCommonData/useInstancing, b1) を必ず読むため、2D描画でも
	// param[10] が初期化済みである必要がある。DrawSettingsCommon でこの既定CBV(=0)を
	// バインドして GBV の「未初期化ルート引数」エラーを防ぐ。
	Microsoft::WRL::ComPtr<ID3D12Resource> defaultCommonDataResource_;

	int blenderMode_; // ブレンダーモード
};

} // namespace TuboEngine