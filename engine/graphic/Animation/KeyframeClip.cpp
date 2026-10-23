#include "KeyframeClip.h"
#include "Animator.h"
#include "JsonManager.h"
#include "Object3d.h"
#include "Sprite.h"
#include "TextObject.h"
#include <algorithm>
#include <cmath>

// =============================================================================
//  JSON の形式
//  {
//    "name": "door_open", "length": 2.0, "loop": false,
//    "tracks": [
//      { "property": "position",
//        "keys": [ { "time": 0.0, "value": [0, 0, 0, 0], "ease": "InOutSine" },
//                  { "time": 1.0, "value": [0, 3, 0, 0], "ease": "OutBack" } ] }
//    ]
//  }
// =============================================================================

namespace TuboEngine {

using Json = JsonManager::Json;

namespace {
// 同じ時間とみなす差（これより近いキーは上書き）
constexpr float kSameTime = 1.0f / 240.0f;

Math::Vector4 Lerp4(const Math::Vector4& a, const Math::Vector4& b, float t) { return a + (b - a) * t; }
} // namespace

// -----------------------------------------------------------------------------
// KeyframeTrack
// -----------------------------------------------------------------------------
bool KeyframeTrack::Evaluate(float time, Math::Vector4& out) const {
	if (keys.empty()) {
		return false;
	}
	if (time <= keys.front().time) {
		out = keys.front().value;
		return true;
	}
	if (time >= keys.back().time) {
		out = keys.back().value;
		return true;
	}
	// time を挟む2つのキーを探す（keys は時間順）
	auto next = std::upper_bound(keys.begin(), keys.end(), time, [](float t, const Keyframe& k) { return t < k.time; });
	const Keyframe& b = *next;
	const Keyframe& a = *(next - 1);
	const float span = b.time - a.time;
	const float t = span > 0.0f ? (time - a.time) / span : 1.0f;
	out = Lerp4(a.value, b.value, Easing::Apply(a.ease, t));
	return true;
}

size_t KeyframeTrack::SetKey(float time, const Math::Vector4& value, Easing::Type ease) {
	for (size_t i = 0; i < keys.size(); ++i) {
		if (std::fabs(keys[i].time - time) < kSameTime) {
			keys[i].value = value;
			return i;
		}
	}
	keys.push_back(Keyframe{time, value, ease});
	Sort();
	for (size_t i = 0; i < keys.size(); ++i) {
		if (keys[i].time == time) {
			return i;
		}
	}
	return keys.size() - 1;
}

void KeyframeTrack::Sort() {
	std::stable_sort(keys.begin(), keys.end(), [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
}

// -----------------------------------------------------------------------------
// KeyframeClip
// -----------------------------------------------------------------------------
KeyframeTrack* KeyframeClip::FindTrack(const std::string& property) {
	for (KeyframeTrack& t : tracks) {
		if (t.property == property) {
			return &t;
		}
	}
	return nullptr;
}

const KeyframeTrack* KeyframeClip::FindTrack(const std::string& property) const {
	for (const KeyframeTrack& t : tracks) {
		if (t.property == property) {
			return &t;
		}
	}
	return nullptr;
}

KeyframeTrack& KeyframeClip::GetOrAddTrack(const std::string& property) {
	if (KeyframeTrack* t = FindTrack(property)) {
		return *t;
	}
	tracks.push_back(KeyframeTrack{property, {}});
	return tracks.back();
}

void KeyframeClip::SetKey(const std::string& property, float time, const Math::Vector4& value, Easing::Type ease) {
	GetOrAddTrack(property).SetKey(time, value, ease);
	length = (std::max)(length, time);
}

bool KeyframeClip::Evaluate(const std::string& property, float time, Math::Vector4& out) const {
	const KeyframeTrack* t = FindTrack(property);
	return t && t->Evaluate(time, out);
}

float KeyframeClip::GetLastKeyTime() const {
	float last = 0.0f;
	for (const KeyframeTrack& t : tracks) {
		if (!t.keys.empty()) {
			last = (std::max)(last, t.keys.back().time);
		}
	}
	return last;
}

bool KeyframeClip::LoadFromJson(const std::string& path) {
	Json root;
	if (!JsonManager::Load(path, root) || !root.is_object()) {
		return false;
	}
	KeyframeClip loaded;
	JsonManager::Read(root, "name", loaded.name);
	JsonManager::Read(root, "length", loaded.length);
	JsonManager::Read(root, "loop", loaded.loop);
	if (root.contains("tracks") && root["tracks"].is_array()) {
		for (const Json& jt : root["tracks"]) {
			KeyframeTrack track;
			JsonManager::Read(jt, "property", track.property);
			if (track.property.empty()) {
				continue;
			}
			if (jt.contains("keys") && jt["keys"].is_array()) {
				for (const Json& jk : jt["keys"]) {
					Keyframe key;
					std::string ease = Easing::ToString(key.ease);
					JsonManager::Read(jk, "time", key.time);
					JsonManager::Read(jk, "value", key.value);
					JsonManager::Read(jk, "ease", ease);
					key.ease = Easing::FromString(ease);
					track.keys.push_back(key);
				}
			}
			track.Sort();
			loaded.tracks.push_back(std::move(track));
		}
	}
	*this = std::move(loaded);
	return true;
}

bool KeyframeClip::SaveToJson(const std::string& path) const {
	Json root;
	root["name"] = name;
	root["length"] = length;
	root["loop"] = loop;
	root["tracks"] = Json::array();
	for (const KeyframeTrack& t : tracks) {
		Json jt;
		jt["property"] = t.property;
		jt["keys"] = Json::array();
		for (const Keyframe& k : t.keys) {
			Json jk;
			jk["time"] = k.time;
			jk["value"] = k.value;
			jk["ease"] = Easing::ToString(k.ease);
			jt["keys"].push_back(jk);
		}
		root["tracks"].push_back(jt);
	}
	return JsonManager::Save(path, root);
}

// -----------------------------------------------------------------------------
// KeyframePlayer
// -----------------------------------------------------------------------------
void KeyframePlayer::Update(float deltaTime) {
	if (!clip_ || !playing_) {
		return;
	}
	time_ += deltaTime * speed_;
	const float length = (std::max)(clip_->length, 0.0001f);
	if (clip_->loop) {
		time_ = std::fmod(time_, length);
		if (time_ < 0.0f) {
			time_ += length;
		}
	} else if (time_ >= length || time_ < 0.0f) {
		time_ = std::clamp(time_, 0.0f, length);
		playing_ = false;
		finished_ = true;
	}
}

bool KeyframePlayer::Evaluate(const std::string& property, Math::Vector4& out) const { return clip_ && clip_->Evaluate(property, time_, out); }

void KeyframePlayer::Apply(Object3d* object) const {
	if (!object) {
		return;
	}
	Math::Vector4 v;
	if (Evaluate("position", v)) object->SetPosition({v.x, v.y, v.z});
	if (Evaluate("rotation", v)) object->SetRotation({v.x, v.y, v.z});
	if (Evaluate("scale", v)) object->SetScale({v.x, v.y, v.z});
	if (Evaluate("color", v)) object->SetModelColor(v);
}

void KeyframePlayer::Apply(Animator* animator) const {
	if (!animator) {
		return;
	}
	Math::Vector4 v;
	if (Evaluate("position", v)) animator->SetPosition({v.x, v.y, v.z});
	if (Evaluate("rotation", v)) animator->SetRotation({v.x, v.y, v.z});
	if (Evaluate("scale", v)) animator->SetScale({v.x, v.y, v.z});
	if (Evaluate("color", v)) animator->SetModelColor(v);
}

void KeyframePlayer::Apply(Sprite* sprite) const {
	if (!sprite) {
		return;
	}
	Math::Vector4 v;
	if (Evaluate("position", v)) sprite->SetPosition({v.x, v.y});
	if (Evaluate("rotation", v)) sprite->SetRotation(v.z);
	if (Evaluate("scale", v)) sprite->SetSize({v.x, v.y});
	if (Evaluate("color", v)) sprite->SetColor(v);
}

void KeyframePlayer::Apply(TextObject* text) const {
	if (!text) {
		return;
	}
	Math::Vector4 v;
	if (Evaluate("position", v)) text->SetPosition({v.x, v.y});
	if (Evaluate("scale", v)) text->SetScale(v.x);
	if (Evaluate("color", v)) text->SetColor(v);
}

} // namespace TuboEngine
