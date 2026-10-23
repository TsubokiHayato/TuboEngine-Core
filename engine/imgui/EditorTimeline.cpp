#include "EditorTimeline.h"
#include "EditorObjects.h"
#include "KeyframeClip.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif // USE_IMGUI
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

namespace TuboEngine::Editor {

#ifdef USE_IMGUI
namespace {
constexpr float kRadToDeg = 57.29577951f;

// オブジェクトごとの編集中クリップ
struct ClipEntry {
	KeyframeClip clip;
	char path[260] = {};
};

struct TimelineState {
	std::map<std::string, ClipEntry> clips;
	float time = 0.0f;
	bool playing = false;
	// 選択中のキー
	int selectedTrack = -1;
	int selectedKey = -1;
	// ドラッグ
	bool scrubbing = false;
	bool draggingKey = false;
	// キーを打つプロパティ
	bool keyPosition = true;
	bool keyRotation = true;
	bool keyScale = true;
	bool keyColor = false;
	Easing::Type newKeyEase = Easing::Type::InOutSine;
	// 右クリックしたキー
	int popupTrack = -1;
	int popupKey = -1;
};
TimelineState& State() {
	static TimelineState state;
	return state;
}

// ファイル名に使えない文字を置き換える
std::string SafeFileName(const std::string& name) {
	std::string out = name;
	for (char& c : out) {
		if (std::strchr("\\/:*?\"<>| ", c)) {
			c = '_';
		}
	}
	return out;
}

const char* PropertyLabel(const std::string& property) {
	if (property == "position") return "位置";
	if (property == "rotation") return "回転";
	if (property == "scale") return "拡縮";
	if (property == "color") return "色";
	return property.c_str();
}

// オブジェクトの今の値を取り出す（そのプロパティを持っていなければ false）
bool ReadProperty(const EditorObject& e, const std::string& property, Math::Vector4& out) {
	auto from3 = [&](const Math::Vector3& v) { out = {v.x, v.y, v.z, 0.0f}; };
	if (property == "position" && e.getPosition) { from3(e.getPosition()); return true; }
	if (property == "rotation" && e.getRotation) { from3(e.getRotation()); return true; }
	if (property == "scale" && e.getScale) { from3(e.getScale()); return true; }
	if (property == "color" && e.getColor) { out = e.getColor(); return true; }
	return false;
}

// クリップの time の値をオブジェクトに入れる
void ApplyClip(EditorObject& e, const KeyframeClip& clip, float time) {
	for (const KeyframeTrack& t : clip.tracks) {
		Math::Vector4 v;
		if (!t.Evaluate(time, v)) continue;
		const Math::Vector3 v3{v.x, v.y, v.z};
		if (t.property == "position" && e.setPosition) e.setPosition(v3);
		else if (t.property == "rotation" && e.setRotation) e.setRotation(v3);
		else if (t.property == "scale" && e.setScale) e.setScale(v3);
		else if (t.property == "color" && e.setColor) e.setColor(v);
	}
}

// 有効なプロパティ全部に、今の時間でキーを打つ（Blender の I キー）
void InsertKeys(EditorObject& e, KeyframeClip& clip, TimelineState& s) {
	const std::pair<const char*, bool> props[] = {{"position", s.keyPosition}, {"rotation", s.keyRotation}, {"scale", s.keyScale}, {"color", s.keyColor}};
	for (const auto& [property, enabled] : props) {
		Math::Vector4 v;
		if (enabled && ReadProperty(e, property, v)) {
			clip.SetKey(property, s.time, v, s.newKeyEase);
		}
	}
}

// イージングのカーブを小さく描く
void DrawEaseCurve(Easing::Type type) {
	const ImVec2 size(140.0f, 70.0f);
	const ImVec2 p = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(25, 25, 25, 255), 3.0f);
	const float pad = 10.0f;
	const float w = size.x - pad * 2.0f, h = size.y - pad * 2.0f;
	ImVec2 prev;
	for (int i = 0; i <= 60; ++i) {
		const float t = i / 60.0f;
		const ImVec2 cur(p.x + pad + t * w, p.y + pad + (1.0f - Easing::Apply(type, t)) * h);
		if (i > 0) dl->AddLine(prev, cur, IM_COL32(240, 200, 80, 255), 2.0f);
		prev = cur;
	}
	ImGui::Dummy(size);
}

void DeleteKey(KeyframeClip& clip, int track, int key) {
	if (track < 0 || track >= static_cast<int>(clip.tracks.size())) return;
	auto& keys = clip.tracks[track].keys;
	if (key < 0 || key >= static_cast<int>(keys.size())) return;
	keys.erase(keys.begin() + key);
	// キーが無くなったトラックは消す
	if (keys.empty()) {
		clip.tracks.erase(clip.tracks.begin() + track);
	}
}
} // namespace
#endif // USE_IMGUI

void DrawTimelineWindow(bool* open, EditorObject* target) {
#ifdef USE_IMGUI
	TimelineState& s = State();
	if (!ImGui::Begin("Timeline", open)) {
		ImGui::End();
		return;
	}
	if (!target) {
		ImGui::TextDisabled("Hierarchy でオブジェクトを選ぶと、ここでキーフレームアニメーションを作れます。");
		ImGui::TextDisabled("（シーンの ImGuiDraw で TuboEngine::Editor::Inspect(\"名前\", obj) したものが選べます）");
		s.playing = false;
		ImGui::End();
		return;
	}

	ClipEntry& entry = s.clips[target->name];
	KeyframeClip& clip = entry.clip;
	if (entry.path[0] == '\0') {
		std::snprintf(entry.path, sizeof(entry.path), "Resources/Animations/%s.json", SafeFileName(target->name).c_str());
		clip.name = target->name;
	}
	bool timeChanged = false;
	const ImGuiIO& io = ImGui::GetIO();

	// --- 上段: 再生 / 時間 / 長さ / キーを打つ ---
	ImGui::Text("%s", target->name.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", target->type);
	ImGui::SameLine();
	if (ImGui::Button(s.playing ? "一時停止" : "再生")) {
		s.playing = !s.playing;
		if (s.playing && s.time >= clip.length) s.time = 0.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button("先頭")) {
		s.time = 0.0f;
		timeChanged = true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90.0f);
	if (ImGui::DragFloat("時間", &s.time, 0.01f, 0.0f, clip.length, "%.2f s")) timeChanged = true;
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80.0f);
	ImGui::DragFloat("長さ", &clip.length, 0.05f, 0.1f, 600.0f, "%.2f s");
	clip.length = (std::max)(clip.length, 0.1f);
	ImGui::SameLine();
	ImGui::Checkbox("ループ", &clip.loop);

	if (target->getPosition) { ImGui::Checkbox("位置", &s.keyPosition); ImGui::SameLine(); }
	if (target->getRotation) { ImGui::Checkbox("回転", &s.keyRotation); ImGui::SameLine(); }
	if (target->getScale) { ImGui::Checkbox("拡縮", &s.keyScale); ImGui::SameLine(); }
	if (target->getColor) { ImGui::Checkbox("色", &s.keyColor); ImGui::SameLine(); }
	int ease = static_cast<int>(s.newKeyEase);
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::Combo("補間", &ease, Easing::Names(), static_cast<int>(Easing::Type::Count))) s.newKeyEase = static_cast<Easing::Type>(ease);
	ImGui::SameLine();
	const bool timelineActive = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
	if (ImGui::Button("キーを打つ (I)") || (timelineActive && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_I, false))) {
		InsertKeys(*target, clip, s);
	}

	// --- 2段目: 保存 / 読み込み ---
	ImGui::SetNextItemWidth(320.0f);
	ImGui::InputText("##path", entry.path, sizeof(entry.path));
	ImGui::SameLine();
	if (ImGui::Button("Save")) {
		clip.SaveToJson(entry.path);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load")) {
		if (clip.LoadFromJson(entry.path)) {
			s.selectedTrack = s.selectedKey = -1;
			timeChanged = true;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("全キー削除")) {
		clip.tracks.clear();
		s.selectedTrack = s.selectedKey = -1;
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip(
		    "ゲームで再生するには:\n"
		    "  TuboEngine::KeyframeClip clip; clip.LoadFromJson(\"%s\");\n"
		    "  TuboEngine::KeyframePlayer player; player.SetClip(&clip); player.Play();\n"
		    "  // Update: player.Update(Time::DeltaTime()); player.Apply(obj);",
		    entry.path);
	}

	// --- 再生 ---
	if (s.playing) {
		s.time += io.DeltaTime;
		if (s.time > clip.length) {
			if (clip.loop) {
				s.time = std::fmod(s.time, clip.length);
			} else {
				s.time = clip.length;
				s.playing = false;
			}
		}
		timeChanged = true;
	}
	s.time = std::clamp(s.time, 0.0f, clip.length);

	// --- タイムライン本体（左: トラック名 / 右: 目盛りと ◆）---
	const float labelWidth = 90.0f;
	const float rulerHeight = 22.0f;
	const float rowHeight = 22.0f;
	const int rows = (std::max)(static_cast<int>(clip.tracks.size()), 1);
	const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	const ImVec2 canvasSize((std::max)(ImGui::GetContentRegionAvail().x, 200.0f), rulerHeight + rows * rowHeight + 4.0f);
	ImGui::InvisibleButton("##timeline_canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	const bool canvasHovered = ImGui::IsItemHovered();

	ImDrawList* dl = ImGui::GetWindowDrawList();
	const float areaLeft = canvasPos.x + labelWidth + 8.0f;
	const float areaRight = canvasPos.x + canvasSize.x - 8.0f;
	const float pixelsPerSecond = (areaRight - areaLeft) / clip.length;
	auto timeToX = [&](float t) { return areaLeft + t * pixelsPerSecond; };
	auto xToTime = [&](float x) { return std::clamp((x - areaLeft) / pixelsPerSecond, 0.0f, clip.length); };

	dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(32, 32, 32, 255));
	dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + rulerHeight), IM_COL32(45, 45, 45, 255));

	// 目盛り（長さに合わせて間隔を変える）
	const float minor = clip.length <= 3.0f ? 0.1f : (clip.length <= 12.0f ? 0.5f : 1.0f);
	const float major = minor * 5.0f;
	for (float t = 0.0f; t <= clip.length + 1e-4f; t += minor) {
		const float x = timeToX(t);
		const bool isMajor = std::fabs(std::remainder(t, major)) < minor * 0.5f;
		dl->AddLine(ImVec2(x, canvasPos.y + (isMajor ? 6.0f : 14.0f)), ImVec2(x, canvasPos.y + rulerHeight), IM_COL32(120, 120, 120, 255));
		if (isMajor) {
			char label[16];
			std::snprintf(label, sizeof(label), "%.1f", t);
			dl->AddText(ImVec2(x + 2.0f, canvasPos.y + 1.0f), IM_COL32(170, 170, 170, 255), label);
			dl->AddLine(ImVec2(x, canvasPos.y + rulerHeight), ImVec2(x, canvasPos.y + canvasSize.y), IM_COL32(50, 50, 50, 255));
		}
	}

	// トラックと ◆
	const ImVec2 mouse = io.MousePos;
	int hoveredTrack = -1, hoveredKey = -1;
	for (int ti = 0; ti < static_cast<int>(clip.tracks.size()); ++ti) {
		const KeyframeTrack& track = clip.tracks[ti];
		const float rowTop = canvasPos.y + rulerHeight + ti * rowHeight;
		const float cy = rowTop + rowHeight * 0.5f;
		if (ti % 2 == 1) {
			dl->AddRectFilled(ImVec2(canvasPos.x, rowTop), ImVec2(canvasPos.x + canvasSize.x, rowTop + rowHeight), IM_COL32(38, 38, 38, 255));
		}
		dl->AddText(ImVec2(canvasPos.x + 6.0f, cy - ImGui::GetFontSize() * 0.5f), IM_COL32(210, 210, 210, 255), PropertyLabel(track.property));
		for (int ki = 0; ki < static_cast<int>(track.keys.size()); ++ki) {
			const float x = timeToX(track.keys[ki].time);
			const float r = 6.0f;
			const bool selected = ti == s.selectedTrack && ki == s.selectedKey;
			const bool hover = canvasHovered && std::fabs(mouse.x - x) <= r + 1.0f && std::fabs(mouse.y - cy) <= r + 1.0f;
			if (hover) {
				hoveredTrack = ti;
				hoveredKey = ki;
			}
			const ImU32 col = selected ? IM_COL32(255, 200, 60, 255) : (hover ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255));
			dl->AddQuadFilled(ImVec2(x, cy - r), ImVec2(x + r, cy), ImVec2(x, cy + r), ImVec2(x - r, cy), col);
			dl->AddQuad(ImVec2(x, cy - r), ImVec2(x + r, cy), ImVec2(x, cy + r), ImVec2(x - r, cy), IM_COL32(20, 20, 20, 255));
		}
	}
	if (clip.tracks.empty()) {
		dl->AddText(ImVec2(areaLeft, canvasPos.y + rulerHeight + 3.0f), IM_COL32(130, 130, 130, 255), "キーがありません（I でキーを打つ）");
	}

	// マウス操作: ◆ をクリックで選択＆ドラッグ開始、それ以外はシーク
	if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		if (hoveredKey >= 0) {
			s.selectedTrack = hoveredTrack;
			s.selectedKey = hoveredKey;
			s.draggingKey = true;
		} else if (mouse.x >= areaLeft - 6.0f) {
			s.scrubbing = true;
			s.playing = false;
		}
	}
	if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hoveredKey >= 0) {
		s.popupTrack = hoveredTrack;
		s.popupKey = hoveredKey;
		s.selectedTrack = hoveredTrack;
		s.selectedKey = hoveredKey;
		ImGui::OpenPopup("TimelineKeyMenu");
	}
	if (s.scrubbing) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			s.time = xToTime(mouse.x);
			timeChanged = true;
		} else {
			s.scrubbing = false;
		}
	}
	if (s.draggingKey) {
		const bool valid = s.selectedTrack >= 0 && s.selectedTrack < static_cast<int>(clip.tracks.size()) && s.selectedKey >= 0 &&
		                   s.selectedKey < static_cast<int>(clip.tracks[s.selectedTrack].keys.size());
		if (valid && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			float t = xToTime(mouse.x);
			if (!io.KeyShift) {
				t = std::round(t * 60.0f) / 60.0f; // 1/60 秒に吸着
			}
			clip.tracks[s.selectedTrack].keys[s.selectedKey].time = t;
			s.time = t;
			timeChanged = true;
		} else {
			s.draggingKey = false;
			if (valid) {
				// 時間順に並べ直し、動かしたキーを選び直す
				KeyframeTrack& track = clip.tracks[s.selectedTrack];
				const Keyframe moved = track.keys[s.selectedKey];
				track.Sort();
				for (int i = 0; i < static_cast<int>(track.keys.size()); ++i) {
					if (track.keys[i].time == moved.time && track.keys[i].value.x == moved.value.x && track.keys[i].value.y == moved.value.y) {
						s.selectedKey = i;
						break;
					}
				}
			}
		}
	}

	// 再生位置の線
	const float nowX = timeToX(s.time);
	dl->AddLine(ImVec2(nowX, canvasPos.y), ImVec2(nowX, canvasPos.y + canvasSize.y), IM_COL32(230, 80, 80, 255), 2.0f);
	dl->AddTriangleFilled(ImVec2(nowX - 5.0f, canvasPos.y), ImVec2(nowX + 5.0f, canvasPos.y), ImVec2(nowX, canvasPos.y + 7.0f), IM_COL32(230, 80, 80, 255));

	// 右クリックメニュー
	if (ImGui::BeginPopup("TimelineKeyMenu")) {
		const bool valid = s.popupTrack >= 0 && s.popupTrack < static_cast<int>(clip.tracks.size()) && s.popupKey >= 0 &&
		                   s.popupKey < static_cast<int>(clip.tracks[s.popupTrack].keys.size());
		if (valid) {
			Keyframe& key = clip.tracks[s.popupTrack].keys[s.popupKey];
			if (ImGui::BeginMenu("補間")) {
				for (int i = 0; i < static_cast<int>(Easing::Type::Count); ++i) {
					if (ImGui::MenuItem(Easing::Names()[i], nullptr, static_cast<int>(key.ease) == i)) {
						key.ease = static_cast<Easing::Type>(i);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("削除", "Delete")) {
				DeleteKey(clip, s.popupTrack, s.popupKey);
				s.selectedTrack = s.selectedKey = -1;
			}
		}
		ImGui::EndPopup();
	}
	if (timelineActive && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false) && s.selectedKey >= 0) {
		DeleteKey(clip, s.selectedTrack, s.selectedKey);
		s.selectedTrack = s.selectedKey = -1;
	}

	// --- 選択中のキーの詳細 ---
	const bool hasSelection = s.selectedTrack >= 0 && s.selectedTrack < static_cast<int>(clip.tracks.size()) && s.selectedKey >= 0 &&
	                          s.selectedKey < static_cast<int>(clip.tracks[s.selectedTrack].keys.size());
	if (hasSelection) {
		KeyframeTrack& track = clip.tracks[s.selectedTrack];
		Keyframe& key = track.keys[s.selectedKey];
		ImGui::SeparatorText("選択中のキー");
		ImGui::BeginGroup();
		ImGui::Text("%s  #%d", PropertyLabel(track.property), s.selectedKey);
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::DragFloat("時間##key", &key.time, 0.01f, 0.0f, clip.length, "%.3f s")) {
			track.Sort();
			s.selectedTrack = s.selectedKey = -1; // 並びが変わるので選択を外す
		} else {
			ImGui::SetNextItemWidth(260.0f);
			if (track.property == "rotation") {
				Math::Vector3 deg{key.value.x * kRadToDeg, key.value.y * kRadToDeg, key.value.z * kRadToDeg};
				if (ImGui::DragFloat3("値 (deg)", &deg.x, 0.5f)) {
					key.value = {deg.x / kRadToDeg, deg.y / kRadToDeg, deg.z / kRadToDeg, 0.0f};
					timeChanged = true;
				}
			} else if (track.property == "color") {
				if (ImGui::ColorEdit4("値", &key.value.x)) timeChanged = true;
			} else {
				if (ImGui::DragFloat3("値", &key.value.x, 0.05f)) timeChanged = true;
			}
			int keyEase = static_cast<int>(key.ease);
			ImGui::SetNextItemWidth(160.0f);
			if (ImGui::Combo("次のキーへの補間", &keyEase, Easing::Names(), static_cast<int>(Easing::Type::Count))) {
				key.ease = static_cast<Easing::Type>(keyEase);
				timeChanged = true;
			}
			if (ImGui::Button("今の値で上書き")) {
				Math::Vector4 v;
				if (ReadProperty(*target, track.property, v)) key.value = v;
			}
			ImGui::SameLine();
			if (ImGui::Button("削除")) {
				DeleteKey(clip, s.selectedTrack, s.selectedKey);
				s.selectedTrack = s.selectedKey = -1;
			}
		}
		ImGui::EndGroup();
		if (s.selectedKey >= 0) {
			ImGui::SameLine();
			DrawEaseCurve(clip.tracks[s.selectedTrack].keys[s.selectedKey].ease);
		}
	}

	// 時間が変わった時だけオブジェクトに反映する（Inspector / Gizmo で動かしてからキーを打てるように）
	if (timeChanged) {
		ApplyClip(*target, clip, s.time);
	}
	ImGui::End();
#else
	(void)open;
	(void)target;
#endif // USE_IMGUI
}

} // namespace TuboEngine::Editor
