#pragma once
#include<memory>
#include"Object/PSO.h"
#include"BlendPSO.h"

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

	int blenderMode_; // ブレンダーモード
};

} // namespace TuboEngine