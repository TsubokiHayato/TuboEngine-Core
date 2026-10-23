#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>

namespace TuboEngine {

/// <summary>
/// フレーム間の経過時間（deltaTime）を管理するクラス（全て static）。
/// Framework::Update が毎フレーム1回 Update() を呼ぶので、ゲーム側は取得するだけでよい。
/// </summary>
/// <remarks>
/// 使い方:
///   position += velocity * TuboEngine::Time::DeltaTime();
///   TuboEngine::Time::SetTimeScale(0.5f); // スロー
/// エディタの一時停止中は DeltaTime() が 0 になる（UnscaledDeltaTime() は進む）。
/// </remarks>
class Time {
public:
	/// <summary>
	/// 経過時間を計測する。Framework::Update から毎フレーム1回呼ぶ。
	/// </summary>
	/// <param name="advance">false ならこのフレームは時間を進めない（一時停止中）</param>
	static void Update(bool advance = true) {
		const Clock::time_point now = Clock::now();
		if (!started_) {
			// 初回は計測の基準が無いので 60FPS 相当とする
			started_ = true;
			unscaledDeltaTime_ = kDefaultDeltaTime;
		} else {
			unscaledDeltaTime_ = std::chrono::duration<float>(now - last_).count();
			// ブレークポイントやウィンドウのドラッグで止まった分を一気に進めないよう上限を設ける
			unscaledDeltaTime_ = (std::min)(unscaledDeltaTime_, kMaxDeltaTime);
		}
		last_ = now;

		deltaTime_ = advance ? unscaledDeltaTime_ * timeScale_ : 0.0f;
		totalTime_ += deltaTime_;
		unscaledTotalTime_ += unscaledDeltaTime_;
		++frameCount_;
	}

	/// <summary>
	/// 前フレームからの経過秒（timeScale・一時停止の影響を受ける）。ゲームの処理はこれを使う。
	/// </summary>
	static float DeltaTime() { return deltaTime_; }
	/// <summary>
	/// 前フレームからの経過秒（timeScale・一時停止の影響を受けない）。ポーズ中も動く UI 演出用。
	/// </summary>
	static float UnscaledDeltaTime() { return unscaledDeltaTime_; }

	/// <summary>
	/// 時間の進む速さ（1.0 で等速、0.5 でスロー、0 で停止）。
	/// </summary>
	static void SetTimeScale(float scale) { timeScale_ = (std::max)(scale, 0.0f); }
	static float GetTimeScale() { return timeScale_; }

	/// <summary>
	/// 起動からの経過秒（timeScale の影響を受ける）。
	/// </summary>
	static float TotalTime() { return totalTime_; }
	/// <summary>
	/// 起動からの経過秒（timeScale の影響を受けない）。
	/// </summary>
	static float UnscaledTotalTime() { return unscaledTotalTime_; }
	/// <summary>
	/// 起動からのフレーム数。
	/// </summary>
	static uint64_t FrameCount() { return frameCount_; }

	// 想定フレームレートでの1フレームの秒数
	static constexpr float kDefaultDeltaTime = 1.0f / 60.0f;
	// 1フレームで進める最大の秒数
	static constexpr float kMaxDeltaTime = 0.1f;

private:
	using Clock = std::chrono::steady_clock;

	static inline Clock::time_point last_{};
	static inline bool started_ = false;
	static inline float deltaTime_ = kDefaultDeltaTime;
	static inline float unscaledDeltaTime_ = kDefaultDeltaTime;
	static inline float timeScale_ = 1.0f;
	static inline float totalTime_ = 0.0f;
	static inline float unscaledTotalTime_ = 0.0f;
	static inline uint64_t frameCount_ = 0;
};

} // namespace TuboEngine
