#pragma once
#include <cmath>
#include <cstddef>
#include <string>

// =============================================================================
//  Easing ── 演出用のイージング関数まとめ
//
//  t は 0〜1（範囲外は内部でクランプする）。戻り値は基本 0〜1 だが、
//  Back / Elastic は途中で 0〜1 をはみ出す（行き過ぎて戻る動き）。
//
//  使い方:
//    float e = TuboEngine::Easing::OutCubic(t);                     // 個別に呼ぶ
//    float e = TuboEngine::Easing::Apply(Easing::Type::OutBack, t); // 種類を変数で選ぶ
//    Vector3 p = TuboEngine::Easing::Ease(start, end, t, Easing::Type::InOutSine);
//  時間を自分で数えたくない場合は Tween.h を使う。
// =============================================================================
namespace TuboEngine::Easing {

/// <summary>
/// イージングの種類。ImGui の Combo や JSON 保存で使えるよう ToString / FromString がある。
/// </summary>
enum class Type {
	Linear,
	InSine, OutSine, InOutSine,
	InQuad, OutQuad, InOutQuad,
	InCubic, OutCubic, InOutCubic,
	InQuart, OutQuart, InOutQuart,
	InQuint, OutQuint, InOutQuint,
	InExpo, OutExpo, InOutExpo,
	InCirc, OutCirc, InOutCirc,
	InBack, OutBack, InOutBack,
	InElastic, OutElastic, InOutElastic,
	InBounce, OutBounce, InOutBounce,
	Count, // 種類の数（ループ用）
};

namespace Detail {
constexpr float kPi = 3.14159265358979f;
constexpr float kBackC1 = 1.70158f;
constexpr float kBackC2 = kBackC1 * 1.525f;
constexpr float kBackC3 = kBackC1 + 1.0f;
constexpr float kElasticC4 = (2.0f * kPi) / 3.0f;
constexpr float kElasticC5 = (2.0f * kPi) / 4.5f;
} // namespace Detail

/// <summary>
/// 0〜1 にクランプする。
/// </summary>
inline float Clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }

inline float Linear(float t) { return Clamp01(t); }

// --- Sine ---
inline float InSine(float t) { t = Clamp01(t); return 1.0f - std::cos((t * Detail::kPi) / 2.0f); }
inline float OutSine(float t) { t = Clamp01(t); return std::sin((t * Detail::kPi) / 2.0f); }
inline float InOutSine(float t) { t = Clamp01(t); return -(std::cos(Detail::kPi * t) - 1.0f) / 2.0f; }

// --- Quad ---
inline float InQuad(float t) { t = Clamp01(t); return t * t; }
inline float OutQuad(float t) { t = Clamp01(t); return 1.0f - (1.0f - t) * (1.0f - t); }
inline float InOutQuad(float t) { t = Clamp01(t); return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f; }

// --- Cubic ---
inline float InCubic(float t) { t = Clamp01(t); return t * t * t; }
inline float OutCubic(float t) { t = Clamp01(t); float u = 1.0f - t; return 1.0f - u * u * u; }
inline float InOutCubic(float t) { t = Clamp01(t); return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f; }

// --- Quart ---
inline float InQuart(float t) { t = Clamp01(t); return t * t * t * t; }
inline float OutQuart(float t) { t = Clamp01(t); return 1.0f - std::pow(1.0f - t, 4.0f); }
inline float InOutQuart(float t) { t = Clamp01(t); return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f; }

// --- Quint ---
inline float InQuint(float t) { t = Clamp01(t); return t * t * t * t * t; }
inline float OutQuint(float t) { t = Clamp01(t); return 1.0f - std::pow(1.0f - t, 5.0f); }
inline float InOutQuint(float t) { t = Clamp01(t); return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f; }

// --- Expo ---
inline float InExpo(float t) { t = Clamp01(t); return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f); }
inline float OutExpo(float t) { t = Clamp01(t); return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
inline float InOutExpo(float t) {
	t = Clamp01(t);
	if (t == 0.0f || t == 1.0f) {
		return t;
	}
	return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

// --- Circ ---
inline float InCirc(float t) { t = Clamp01(t); return 1.0f - std::sqrt(1.0f - t * t); }
inline float OutCirc(float t) { t = Clamp01(t); return std::sqrt(1.0f - (t - 1.0f) * (t - 1.0f)); }
inline float InOutCirc(float t) {
	t = Clamp01(t);
	return t < 0.5f ? (1.0f - std::sqrt(1.0f - std::pow(2.0f * t, 2.0f))) / 2.0f : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}

// --- Back（少し行き過ぎて戻る）---
inline float InBack(float t) { t = Clamp01(t); return Detail::kBackC3 * t * t * t - Detail::kBackC1 * t * t; }
inline float OutBack(float t) { t = Clamp01(t); float u = t - 1.0f; return 1.0f + Detail::kBackC3 * u * u * u + Detail::kBackC1 * u * u; }
inline float InOutBack(float t) {
	t = Clamp01(t);
	using namespace Detail;
	return t < 0.5f ? (std::pow(2.0f * t, 2.0f) * ((kBackC2 + 1.0f) * 2.0f * t - kBackC2)) / 2.0f
	                : (std::pow(2.0f * t - 2.0f, 2.0f) * ((kBackC2 + 1.0f) * (t * 2.0f - 2.0f) + kBackC2) + 2.0f) / 2.0f;
}

// --- Elastic（バネのように揺れる）---
inline float InElastic(float t) {
	t = Clamp01(t);
	if (t == 0.0f || t == 1.0f) {
		return t;
	}
	return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * Detail::kElasticC4);
}
inline float OutElastic(float t) {
	t = Clamp01(t);
	if (t == 0.0f || t == 1.0f) {
		return t;
	}
	return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * Detail::kElasticC4) + 1.0f;
}
inline float InOutElastic(float t) {
	t = Clamp01(t);
	if (t == 0.0f || t == 1.0f) {
		return t;
	}
	return t < 0.5f ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * Detail::kElasticC5)) / 2.0f
	                : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * Detail::kElasticC5)) / 2.0f + 1.0f;
}

// --- Bounce（跳ねて止まる）---
inline float OutBounce(float t) {
	t = Clamp01(t);
	constexpr float n1 = 7.5625f;
	constexpr float d1 = 2.75f;
	if (t < 1.0f / d1) {
		return n1 * t * t;
	} else if (t < 2.0f / d1) {
		t -= 1.5f / d1;
		return n1 * t * t + 0.75f;
	} else if (t < 2.5f / d1) {
		t -= 2.25f / d1;
		return n1 * t * t + 0.9375f;
	}
	t -= 2.625f / d1;
	return n1 * t * t + 0.984375f;
}
inline float InBounce(float t) { return 1.0f - OutBounce(1.0f - Clamp01(t)); }
inline float InOutBounce(float t) {
	t = Clamp01(t);
	return t < 0.5f ? (1.0f - OutBounce(1.0f - 2.0f * t)) / 2.0f : (1.0f + OutBounce(2.0f * t - 1.0f)) / 2.0f;
}

/// <summary>
/// 種類を指定してイージングを計算する。
/// </summary>
inline float Apply(Type type, float t) {
	switch (type) {
	case Type::InSine:       return InSine(t);
	case Type::OutSine:      return OutSine(t);
	case Type::InOutSine:    return InOutSine(t);
	case Type::InQuad:       return InQuad(t);
	case Type::OutQuad:      return OutQuad(t);
	case Type::InOutQuad:    return InOutQuad(t);
	case Type::InCubic:      return InCubic(t);
	case Type::OutCubic:     return OutCubic(t);
	case Type::InOutCubic:   return InOutCubic(t);
	case Type::InQuart:      return InQuart(t);
	case Type::OutQuart:     return OutQuart(t);
	case Type::InOutQuart:   return InOutQuart(t);
	case Type::InQuint:      return InQuint(t);
	case Type::OutQuint:     return OutQuint(t);
	case Type::InOutQuint:   return InOutQuint(t);
	case Type::InExpo:       return InExpo(t);
	case Type::OutExpo:      return OutExpo(t);
	case Type::InOutExpo:    return InOutExpo(t);
	case Type::InCirc:       return InCirc(t);
	case Type::OutCirc:      return OutCirc(t);
	case Type::InOutCirc:    return InOutCirc(t);
	case Type::InBack:       return InBack(t);
	case Type::OutBack:      return OutBack(t);
	case Type::InOutBack:    return InOutBack(t);
	case Type::InElastic:    return InElastic(t);
	case Type::OutElastic:   return OutElastic(t);
	case Type::InOutElastic: return InOutElastic(t);
	case Type::InBounce:     return InBounce(t);
	case Type::OutBounce:    return OutBounce(t);
	case Type::InOutBounce:  return InOutBounce(t);
	case Type::Linear:
	default:                 return Linear(t);
	}
}

/// <summary>
/// start から end へ、イージングをかけて補間する（float / Vector2 / Vector3 / Vector4 など）。
/// T は「T + T」「T - T」「T * float」ができる型であればよい。
/// </summary>
template<typename T> T Ease(const T& start, const T& end, float t, Type type) { return start + (end - start) * Apply(type, t); }

/// <summary>
/// イージングの名前（ImGui 表示・JSON 保存用）。Type の並びと一致させること。
/// </summary>
inline const char* const* Names() {
	static const char* const kNames[] = {
	    "Linear",
	    "InSine",    "OutSine",    "InOutSine",
	    "InQuad",    "OutQuad",    "InOutQuad",
	    "InCubic",   "OutCubic",   "InOutCubic",
	    "InQuart",   "OutQuart",   "InOutQuart",
	    "InQuint",   "OutQuint",   "InOutQuint",
	    "InExpo",    "OutExpo",    "InOutExpo",
	    "InCirc",    "OutCirc",    "InOutCirc",
	    "InBack",    "OutBack",    "InOutBack",
	    "InElastic", "OutElastic", "InOutElastic",
	    "InBounce",  "OutBounce",  "InOutBounce",
	};
	static_assert(sizeof(kNames) / sizeof(kNames[0]) == static_cast<size_t>(Type::Count), "Easing::Names と Type の数が合っていない");
	return kNames;
}

inline const char* ToString(Type type) {
	const int index = static_cast<int>(type);
	if (index < 0 || index >= static_cast<int>(Type::Count)) {
		return "Linear";
	}
	return Names()[index];
}

/// <summary>
/// 名前から種類を得る。見つからなければ Linear。
/// </summary>
inline Type FromString(const std::string& name) {
	for (int i = 0; i < static_cast<int>(Type::Count); ++i) {
		if (name == Names()[i]) {
			return static_cast<Type>(i);
		}
	}
	return Type::Linear;
}

} // namespace TuboEngine::Easing
