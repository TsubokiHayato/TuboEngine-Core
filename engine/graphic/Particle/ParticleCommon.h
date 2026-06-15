#pragma once
#pragma once
#include"WinApp.h"
#include"DirectXCommon.h"
#include"SrvManager.h"
#include"Particle/ParticlePSO.h"
namespace TuboEngine {
class Camera;
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
	ParticleCommon() = default;
	~ParticleCommon() = default;
	ParticleCommon(const ParticleCommon&) = delete;
	ParticleCommon& operator=(const ParticleCommon&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon">DirectX共通部分</param>
	void Initialize();

	void Finalize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawSettingsCommon();

	void SetDefaultCamera(Camera* camera) { defaultCamera = camera; }
	Camera* GetDefaultCamera() const { return defaultCamera; }

private:
	std::unique_ptr<ParticlePSO> pso = nullptr; // PSOのユニークポインタ
	Camera* defaultCamera = nullptr;            // デフォルトカメラ
};

} // namespace TuboEngine