#pragma once
#pragma once
#include"WinApp.h"
#include"DirectXCommon.h"
#include"SrvManager.h"
#include"Particle/ParticlePSO.h"
namespace TuboEngine {
class Camera;
/// <summary>
/// パーティクル描画の共通設定（PSO・ブレンドモード）を管理するクラス。
/// </summary>
class ParticleCommon {

public:
	/// <summary>
	/// シングルトンインスタンス取得
	/// </summary>
	static ParticleCommon* GetInstance() {

		if (!instance) {
			instance = new ParticleCommon();
		}
		return instance;
	}

private:
	// コンストラクタ・デストラクタ・コピー禁止
	static ParticleCommon* instance; // シングルトンインスタンス
	/// <summary>
	/// コンストラクタ。
	/// </summary>
	ParticleCommon() = default;
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~ParticleCommon() = default;
	/// <summary>
	/// コピー禁止。
	/// </summary>
	ParticleCommon(const ParticleCommon&) = delete;
	ParticleCommon& operator=(const ParticleCommon&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon">DirectX共通部分</param>
	void Initialize();

	/// <summary>
	/// 終了処理。
	/// </summary>
	void Finalize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawSettingsCommon();

	/// <summary>
	/// デフォルトカメラの取得・設定。
	/// </summary>
	void SetDefaultCamera(Camera* camera) { defaultCamera = camera; }
	Camera* GetDefaultCamera() const { return defaultCamera; }

private:
	std::unique_ptr<ParticlePSO> pso = nullptr; // PSOのユニークポインタ
	Camera* defaultCamera = nullptr;            // デフォルトカメラ
};

} // namespace TuboEngine