#pragma once
#include "Easing.h"
#include <algorithm>

namespace TuboEngine {

/// <summary>
/// start → end へ、指定時間かけてイージング付きで値を動かすクラス（float / Vector2 / Vector3 / Vector4 など）。
/// 経過時間を自分で数えなくてよい。
/// </summary>
/// <remarks>
/// 使い方:
///   TuboEngine::Tween<Vector2> slideIn;
///   slideIn.Start({-300, 100}, {100, 100}, 0.4f, Easing::Type::OutBack);  // 開始
///   // Update 内
///   sprite->SetPosition(slideIn.Update(Time::DeltaTime()));
///   if (slideIn.IsFinished()) { ... }
/// </remarks>
template<typename T> class Tween {
public:
	enum class LoopMode {
		None,     // 1回で終わる
		Restart,  // 終わったら最初から
		PingPong, // 行って戻ってを繰り返す
	};

	Tween() = default;
	Tween(const T& start, const T& end, float duration, Easing::Type type = Easing::Type::Linear, float delay = 0.0f) { Start(start, end, duration, type, delay); }

	/// <summary>
	/// 開始する（経過時間は 0 に戻る）。
	/// </summary>
	/// <param name="delay">動き出すまでの待ち時間（秒）。待っている間は start を返す</param>
	void Start(const T& start, const T& end, float duration, Easing::Type type = Easing::Type::Linear, float delay = 0.0f) {
		start_ = start;
		end_ = end;
		duration_ = duration;
		type_ = type;
		delay_ = delay;
		Restart();
	}

	/// <summary>
	/// 同じ設定で最初からやり直す。
	/// </summary>
	void Restart() {
		elapsed_ = -delay_;
		reversed_ = false;
		isPlaying_ = true;
		value_ = start_;
	}

	/// <summary>
	/// 時間を進めて現在の値を返す。停止中は最後の値を返す。
	/// </summary>
	const T& Update(float deltaTime) {
		if (!isPlaying_) {
			return value_;
		}
		elapsed_ += deltaTime;
		if (elapsed_ < 0.0f) {
			value_ = start_; // delay 中
			return value_;
		}

		if (duration_ <= 0.0f) {
			value_ = end_;
			isPlaying_ = loop_ != LoopMode::None;
			return value_;
		}

		if (elapsed_ >= duration_) {
			switch (loop_) {
			case LoopMode::None:
				elapsed_ = duration_;
				isPlaying_ = false;
				break;
			case LoopMode::Restart:
				elapsed_ = std::fmod(elapsed_, duration_);
				break;
			case LoopMode::PingPong:
				elapsed_ = std::fmod(elapsed_, duration_);
				reversed_ = !reversed_;
				break;
			}
		}

		const float t = elapsed_ / duration_;
		value_ = reversed_ ? Easing::Ease(end_, start_, t, type_) : Easing::Ease(start_, end_, t, type_);
		return value_;
	}

	/// <summary>
	/// 終点へ飛ばして終了する。
	/// </summary>
	void Complete() {
		elapsed_ = duration_;
		value_ = reversed_ ? start_ : end_;
		isPlaying_ = false;
	}

	void Pause() { isPlaying_ = false; }
	void Resume() { isPlaying_ = true; }

	void SetLoopMode(LoopMode mode) { loop_ = mode; }
	LoopMode GetLoopMode() const { return loop_; }

	const T& GetValue() const { return value_; }
	bool IsPlaying() const { return isPlaying_; }
	// ループしない設定で、最後まで進んだか
	bool IsFinished() const { return !isPlaying_ && loop_ == LoopMode::None && elapsed_ >= duration_; }
	// 0〜1 の進み具合（delay 中は 0）
	float GetProgress() const { return duration_ > 0.0f ? std::clamp(elapsed_ / duration_, 0.0f, 1.0f) : 1.0f; }

private:
	T start_{};
	T end_{};
	T value_{};
	float duration_ = 0.0f;
	float delay_ = 0.0f;
	float elapsed_ = 0.0f;
	Easing::Type type_ = Easing::Type::Linear;
	LoopMode loop_ = LoopMode::None;
	bool reversed_ = false;
	bool isPlaying_ = false;
};

} // namespace TuboEngine
