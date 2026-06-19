#pragma once
#include"Object/PSO.h"
#include"BlendPSO.h"
#include"Camera.h"
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>


class Camera;
namespace TuboEngine {
class Object3dCommon {
public:
	/// <summary>
	/// シングルトンインスタンス取得
	/// </summary>
	static Object3dCommon* GetInstance() {

		if (instance == nullptr) {
			instance = new Object3dCommon();
		}
		return instance;
	}

private:
	// コンストラクタ・デストラクタ・コピー禁止
	static Object3dCommon* instance; // シングルトンインスタンス
	Object3dCommon() = default;
	~Object3dCommon() = default;
	Object3dCommon(const Object3dCommon&) = delete;
	Object3dCommon& operator=(const Object3dCommon&) = delete;

public:
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

	/*---------------------------------------------------
	        GETTER & SETTER
	---------------------------------------------------*/

	void SetDefaultCamera(TuboEngine::Camera* camera) { defaultCamera = camera; }
	TuboEngine::Camera* GetDefaultCamera() const { return defaultCamera; }

private:
	std::unique_ptr<PSO> pso = nullptr;  // PSOのユニークポインタ
	std::unique_ptr<BlendPSO> blendPso_; // ブレンドPSOのユニークポインタ

	// param[10] (VS b1, gCommonData) 用の既定CBV（useInstancing=0）。
	// ルートシグネチャ束縛直後にこれを必ずバインドしておくことで、
	// 個別描画が param[10] をセットし忘れても VS の gCommonData が
	// 初期化済みになり、GBV の「未初期化ルート引数」エラーを防ぐ。
	Microsoft::WRL::ComPtr<ID3D12Resource> defaultCommonDataResource_;

	TuboEngine::Camera* defaultCamera = nullptr; // デフォルトカメラ
	int blenderMode_ = 0;                        // ブレンダーモード
};

} // namespace TuboEngine