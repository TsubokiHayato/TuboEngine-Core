#pragma once
#include "Easing.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include <string>
#include <vector>

namespace TuboEngine {

class Object3d;
class Animator;
class Sprite;
class TextObject;

// =============================================================================
//  キーフレームアニメーション（Blender のタイムライン / ドープシートのように、
//  「この時間にこの値」を打っておき、間をイージングで補間して動かす）
//
//  ・作る   : Debug の Timeline ウィンドウでキーを打って Save（JSON）
//  ・再生する: KeyframeClip を読み込み、KeyframePlayer で時間を進めて対象に Apply
//
//    TuboEngine::KeyframeClip clip;
//    clip.LoadFromJson("Resources/Animations/door_open.json");
//    TuboEngine::KeyframePlayer player;
//    player.SetClip(&clip);
//    player.Play();
//    // Update
//    player.Update(TuboEngine::Time::DeltaTime());
//    player.Apply(door_.get());   // Object3d / Animator / Sprite / TextObject
//
//  プロパティ名: "position" / "rotation" / "scale"（Vector3）, "color"（Vector4）
//    2D は position の x,y を使う。Sprite は rotation の z を回転、scale の x,y をサイズとして、
//    TextObject は scale の x を拡大率として使う。
// =============================================================================

/// <summary>
/// キー1個（時間と値）。ease はこのキーから「次のキー」へ向かう時の補間方法。
/// </summary>
struct Keyframe {
	float time = 0.0f;
	Math::Vector4 value{0.0f, 0.0f, 0.0f, 0.0f}; // Vector3 のプロパティは x,y,z を使う
	Easing::Type ease = Easing::Type::InOutSine;
};

/// <summary>
/// 1つのプロパティ（"position" など）のキーの列。keys は時間順に並ぶ。
/// </summary>
struct KeyframeTrack {
	std::string property;
	std::vector<Keyframe> keys;

	/// <summary>
	/// 時間 time の値を求める（最初のキーより前は最初の値、最後より後は最後の値）。キーが無ければ false。
	/// </summary>
	bool Evaluate(float time, Math::Vector4& out) const;
	/// <summary>
	/// キーを打つ（同じ時間のキーがあれば値を上書き）。打ったキーの番号を返す。
	/// </summary>
	size_t SetKey(float time, const Math::Vector4& value, Easing::Type ease);
	/// <summary>
	/// 時間順に並べ直す（キーの時間を動かした後に呼ぶ）。
	/// </summary>
	void Sort();
};

/// <summary>
/// キーフレームアニメーション1本（複数のプロパティのトラックを持つ）。
/// </summary>
class KeyframeClip {
public:
	std::string name;
	float length = 2.0f; // 長さ（秒）。Timeline の表示範囲・ループの長さ
	bool loop = false;
	std::vector<KeyframeTrack> tracks;

	KeyframeTrack* FindTrack(const std::string& property);
	const KeyframeTrack* FindTrack(const std::string& property) const;
	/// <summary>
	/// トラックを取得する（無ければ作る）。
	/// </summary>
	KeyframeTrack& GetOrAddTrack(const std::string& property);

	/// <summary>
	/// property の time にキーを打つ。
	/// </summary>
	void SetKey(const std::string& property, float time, const Math::Vector4& value, Easing::Type ease = Easing::Type::InOutSine);
	void SetKey(const std::string& property, float time, const Math::Vector3& value, Easing::Type ease = Easing::Type::InOutSine) {
		SetKey(property, time, Math::Vector4{value.x, value.y, value.z, 0.0f}, ease);
	}
	/// <summary>
	/// time の値を求める。そのプロパティのキーが無ければ false。
	/// </summary>
	bool Evaluate(const std::string& property, float time, Math::Vector4& out) const;

	/// <summary>
	/// 一番最後のキーの時間（キーが無ければ 0）。
	/// </summary>
	float GetLastKeyTime() const;

	bool LoadFromJson(const std::string& path);
	bool SaveToJson(const std::string& path) const;
};

/// <summary>
/// KeyframeClip の再生（時間を進めて、対象に値を入れる）。クリップ自体は持たない（ポインタで参照）。
/// </summary>
class KeyframePlayer {
public:
	void SetClip(const KeyframeClip* clip) {
		clip_ = clip;
		time_ = 0.0f;
		finished_ = false;
	}
	const KeyframeClip* GetClip() const { return clip_; }

	void Play() { playing_ = true; }
	void Pause() { playing_ = false; }
	/// <summary>
	/// 止めて先頭に戻す。
	/// </summary>
	void Stop() {
		playing_ = false;
		time_ = 0.0f;
		finished_ = false;
	}
	bool IsPlaying() const { return playing_; }
	/// <summary>
	/// ループしないクリップが最後まで行ったか。
	/// </summary>
	bool IsFinished() const { return finished_; }

	void SetTime(float time) { time_ = time; }
	float GetTime() const { return time_; }
	void SetSpeed(float speed) { speed_ = speed; }

	/// <summary>
	/// 時間を進める（再生中のみ）。
	/// </summary>
	void Update(float deltaTime);

	/// <summary>
	/// 今の時間の値を対象に入れる（クリップに無いプロパティは触らない）。
	/// </summary>
	void Apply(Object3d* object) const;
	void Apply(Animator* animator) const;
	void Apply(Sprite* sprite) const;
	void Apply(TextObject* text) const;
	/// <summary>
	/// 値だけ取り出す（自分で好きな変数に入れたい時）。
	/// </summary>
	bool Evaluate(const std::string& property, Math::Vector4& out) const;

private:
	const KeyframeClip* clip_ = nullptr;
	float time_ = 0.0f;
	float speed_ = 1.0f;
	bool playing_ = false;
	bool finished_ = false;
};

} // namespace TuboEngine
