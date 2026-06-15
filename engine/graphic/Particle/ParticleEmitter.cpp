// Emit.cpp
#include "ParticleEmitter.h"
#include "Camera.h"
#include <cmath>
#include"Matrix.h"
#include "TextureManager.h"
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

// コンストラクタ
	/// <summary>
	/// パーティクルエミッターのコンストラクタ
	/// </summary>
	/// <param name="particle">パーティクルのインスタンス</param>
	/// <param name="name">パーティクルグループ名</param>
	/// <param name="transform">エミッターの位置・回転・スケール</param>
	/// <param name="velocity">速度</param>
	/// <param name="color">カラー</param>
	/// <param name="lifeTime">寿命</param>
	/// 	/// <param name="currentTime">経過時間</param>
	/// <param name="count">発生させるパーティクルの数</param>
	/// 	/// <param name="frequency">発生頻度</param>
	/// 	/// <param name="repeat">繰り返し発生させるかどうかのフラグ</param>
TuboEngine::ParticleEmitter::ParticleEmitter(
    Particle* particle, const std::string& name, const TuboEngine::Transform& transform, TuboEngine::Math::Vector3 velocity, Vector4 color, float lifeTime, float currentTime,
	uint32_t count, float frequency, bool repeat)
	//パーティクルのインスタンス
	: particle_(particle),
	//パーティクルグループ名
	name_(name),
	// エミッターの位置・回転・スケール
	transform_(transform),
	// 速度
	velocity_(velocity),
	// カラー
	color_(color),
	// 寿命
	lifeTime_(lifeTime),
	// 経過時間
	currentTime_(currentTime),
	// 発生させるパーティクルの数
	count_(count),
	// 発生頻度
	frequency_(frequency),
	// 発生頻度
	elapsedTime_(frequency),
	// 繰り返し発生させるかどうかのフラグ
	repeat_(repeat) {
	Emit(); // 初期化時に即時発生
}

void TuboEngine::ParticleEmitter::Update() {
	if (!repeat_) return; // 繰り返し発生させない場合は何もしない

	elapsedTime_ += 1.0f / 60.0f; // フレーム単位の経過時間を加算

	// 経過時間が発生頻度を超えたらパーティクルを発生させる
	if (elapsedTime_ >= frequency_) {
		// パーティクルの発生
		Emit();
		// 周期的に実行するためリセット
		elapsedTime_ -= frequency_;
	}
}

void TuboEngine::ParticleEmitter::Draw() {

}

void TuboEngine::ParticleEmitter::Emit() {
	// パーティクルの発生
	particle_->Emit(name_, transform_, velocity_, color_, lifeTime_, currentTime_, count_);
}

void TuboEngine::ParticleEmitter::SetRepeat(bool repeat) {
	// 繰り返し発生させるかどうかのフラグを設定
	repeat_ = repeat;
}
