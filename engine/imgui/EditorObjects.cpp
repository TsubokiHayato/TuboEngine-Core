#include "EditorObjects.h"
#include "Animator.h"
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"
#include "TextObject.h"
#include "EditorPanels.h"
#include "SceneManager.h"
#include "SceneSaveData.h"
#include "TextureManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h" // ホバー中のウィンドウ判定
#endif // USE_IMGUI
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>

namespace TuboEngine::Editor {

namespace {
std::vector<EditorObject>& Objects() {
	static std::vector<EditorObject> objects;
	return objects;
}

constexpr float kRadToDeg = 57.29577951f;
constexpr float kDegToRad = 1.0f / kRadToDeg;

// Gizmo のドラッグ状態（フレームをまたぐので名前で対象を覚える）
struct GizmoDrag {
	std::string objectName;
	int axis = -1; // -1 = ドラッグしていない
	float startMouseX = 0.0f;
	float startMouseY = 0.0f;
	Math::Vector3 startPosition{};
	float screenDirX = 0.0f; // 軸の画面上の向き（正規化）
	float screenDirY = 0.0f;
	float pixelsPerUnit = 1.0f;
};
GizmoDrag& Drag() {
	static GizmoDrag drag;
	return drag;
}

// シーン開始時（または最後に保存した時）の値。名前ごと。シーンが作り直されたら消す
struct Baselines {
	uint32_t sceneGeneration = 0;
	std::map<std::string, SceneSaveData::ObjectData> values;
};
Baselines& BaselineStore() {
	static Baselines store;
	return store;
}

// オブジェクトの今の値を保存形式にする（持っている項目だけ）
SceneSaveData::ObjectData Snapshot(const EditorObject& e) {
	SceneSaveData::ObjectData d;
	if (e.getPosition) d.position = e.getPosition();
	if (e.getRotation) d.rotation = e.getRotation();
	if (e.getScale) d.scale = e.getScale();
	if (e.getColor) d.color = e.getColor();
	if (e.getTexture) d.texture = e.getTexture();
	return d;
}

bool Near(float a, float b) { return std::fabs(a - b) < 1e-4f; }
bool SameV3(const std::optional<Math::Vector3>& a, const std::optional<Math::Vector3>& b) {
	if (a.has_value() != b.has_value()) return false;
	return !a || (Near(a->x, b->x) && Near(a->y, b->y) && Near(a->z, b->z));
}
bool SameV4(const std::optional<Math::Vector4>& a, const std::optional<Math::Vector4>& b) {
	if (a.has_value() != b.has_value()) return false;
	return !a || (Near(a->x, b->x) && Near(a->y, b->y) && Near(a->z, b->z) && Near(a->w, b->w));
}
bool SameData(const SceneSaveData::ObjectData& a, const SceneSaveData::ObjectData& b) {
	return SameV3(a.position, b.position) && SameV3(a.rotation, b.rotation) && SameV3(a.scale, b.scale) && SameV4(a.color, b.color) && a.texture == b.texture;
}

// 保存値（または起動時の値）をオブジェクトに戻す
void ApplyData(EditorObject& e, const SceneSaveData::ObjectData& d) {
	if (d.position && e.setPosition) e.setPosition(*d.position);
	if (d.rotation && e.setRotation) e.setRotation(*d.rotation);
	if (d.scale && e.setScale) e.setScale(*d.scale);
	if (d.color && e.setColor) e.setColor(*d.color);
	if (d.texture && e.setTexture) e.setTexture(*d.texture);
}

bool HasSavableValues(const EditorObject& e) { return e.getPosition || e.getRotation || e.getScale || e.getColor || e.getTexture; }

// Sprite のテクスチャ差し替え（読み込めなければ false）
bool SetSpriteTexture(Sprite* s, const std::string& name) {
	TextureManager::GetInstance()->LoadTexture(name);
	if (!TextureManager::GetInstance()->HasTexture(name)) {
		return false;
	}
	s->SetTexture(name);
	return true;
}
} // namespace

// -----------------------------------------------------------------------------
// 登録
// -----------------------------------------------------------------------------
void Register(EditorObject object) {
#ifdef USE_IMGUI
	// 同じ名前が既にあれば後から来た方に "(2)" などを付けて区別する
	std::string base = object.name.empty() ? "(no name)" : object.name;
	std::string name = base;
	for (int n = 2; FindObject(name); ++n) {
		name = base + " (" + std::to_string(n) + ")";
	}
	object.name = name;

	// シーンに最初に出てきた時の値を覚える（変更あり * の判定用）
	Baselines& baselines = BaselineStore();
	const uint32_t generation = SceneManager::GetInstance()->GetSceneGeneration();
	if (baselines.sceneGeneration != generation) {
		baselines.sceneGeneration = generation;
		baselines.values.clear();
	}
	if (!baselines.values.contains(name) && HasSavableValues(object)) {
		baselines.values[name] = Snapshot(object);
	}
	Objects().push_back(std::move(object));
#else
	(void)object;
#endif // USE_IMGUI
}

void Inspect(const std::string& name, Object3d* o, std::function<void()> extra) {
#ifdef USE_IMGUI
	if (!o) {
		return;
	}
	EditorObject e;
	e.name = name;
	e.type = "Object3d";
	e.is3D = true;
	e.getPosition = [o] { return o->GetPosition(); };
	e.setPosition = [o](const Math::Vector3& v) { o->SetPosition(v); };
	e.getRotation = [o] { return o->GetRotation(); };
	e.setRotation = [o](const Math::Vector3& v) { o->SetRotation(v); };
	e.getScale = [o] { return o->GetScale(); };
	e.setScale = [o](const Math::Vector3& v) { o->SetScale(v); };
	e.getColor = [o] { return o->GetModelColor(); };
	e.setColor = [o](const Math::Vector4& v) { o->SetModelColor(v); };
	e.getTexture = [o] { return o->GetTexture(); };
	e.setTexture = [o](const std::string& name) { return o->SetTexture(name); };
	e.clearTexture = [o] { o->ClearTexture(); };
	e.drawExtra = std::move(extra);
	Register(std::move(e));
#else
	(void)name; (void)o; (void)extra;
#endif // USE_IMGUI
}

void Inspect(const std::string& name, Animator* a, std::function<void()> extra) {
#ifdef USE_IMGUI
	if (!a) {
		return;
	}
	EditorObject e;
	e.name = name;
	e.type = "Animator";
	e.is3D = true;
	e.getPosition = [a] { return a->GetPosition(); };
	e.setPosition = [a](const Math::Vector3& v) { a->SetPosition(v); };
	e.getRotation = [a] { return a->GetRotation(); };
	e.setRotation = [a](const Math::Vector3& v) { a->SetRotation(v); };
	e.getScale = [a] { return a->GetScale(); };
	e.setScale = [a](const Math::Vector3& v) { a->SetScale(v); };
	e.getColor = [a] { return a->GetModelColor(); };
	e.setColor = [a](const Math::Vector4& v) { a->SetModelColor(v); };
	e.getTexture = [a] { return a->GetObject3d()->GetTexture(); };
	e.setTexture = [a](const std::string& name) { return a->GetObject3d()->SetTexture(name); };
	e.clearTexture = [a] { a->GetObject3d()->ClearTexture(); };
	e.drawExtra = [a, extra = std::move(extra)] {
		ImGui::SeparatorText("Animation");
		ImGui::Text("%s  %.2f / %.2f s", a->GetCurrentAnimationName().c_str(), a->GetAnimationTime(), a->GetDuration());
		if (extra) {
			extra();
		}
	};
	Register(std::move(e));
#else
	(void)name; (void)a; (void)extra;
#endif // USE_IMGUI
}

void Inspect(const std::string& name, Sprite* s, std::function<void()> extra) {
#ifdef USE_IMGUI
	if (!s) {
		return;
	}
	EditorObject e;
	e.name = name;
	e.type = "Sprite";
	// 2D は Vector3 の x,y を位置、z を回転として扱う（Timeline のキーと同じ）
	e.getPosition = [s] { return Math::Vector3{s->GetPosition().x, s->GetPosition().y, 0.0f}; };
	e.setPosition = [s](const Math::Vector3& v) { s->SetPosition({v.x, v.y}); };
	e.getRotation = [s] { return Math::Vector3{0.0f, 0.0f, s->GetRotation()}; };
	e.setRotation = [s](const Math::Vector3& v) { s->SetRotation(v.z); };
	e.getScale = [s] { return Math::Vector3{s->GetSize().x, s->GetSize().y, 1.0f}; };
	e.setScale = [s](const Math::Vector3& v) { s->SetSize({v.x, v.y}); };
	e.getColor = [s] { return s->GetColor(); };
	e.setColor = [s](const Math::Vector4& v) { s->SetColor(v); };
	e.getTexture = [s] { return s->GetTexture(); };
	e.setTexture = [s](const std::string& name) { return SetSpriteTexture(s, name); };
	e.drawExtra = std::move(extra);
	Register(std::move(e));
#else
	(void)name; (void)s; (void)extra;
#endif // USE_IMGUI
}

void Inspect(const std::string& name, TextObject* t, std::function<void()> extra) {
#ifdef USE_IMGUI
	if (!t) {
		return;
	}
	EditorObject e;
	e.name = name;
	e.type = "Text";
	e.getPosition = [t] { return Math::Vector3{t->GetPosition().x, t->GetPosition().y, 0.0f}; };
	e.setPosition = [t](const Math::Vector3& v) { t->SetPosition({v.x, v.y}); };
	e.getScale = [t] { return Math::Vector3{t->GetScale(), t->GetScale(), 1.0f}; };
	e.setScale = [t](const Math::Vector3& v) { t->SetScale(v.x); };
	e.getColor = [t] { return t->GetColor(); };
	e.setColor = [t](const Math::Vector4& v) { t->SetColor(v); };
	e.drawExtra = std::move(extra);
	Register(std::move(e));
#else
	(void)name; (void)t; (void)extra;
#endif // USE_IMGUI
}

void InspectCustom(const std::string& name, std::function<void()> draw) {
#ifdef USE_IMGUI
	EditorObject e;
	e.name = name;
	e.type = "Custom";
	e.drawExtra = std::move(draw);
	Register(std::move(e));
#else
	(void)name; (void)draw;
#endif // USE_IMGUI
}

void BeginFrame() { Objects().clear(); }

const std::vector<EditorObject>& GetObjects() { return Objects(); }

EditorObject* FindObject(const std::string& name) {
	for (EditorObject& e : Objects()) {
		if (e.name == name) {
			return &e;
		}
	}
	return nullptr;
}

bool IsGizmoDragging() { return Drag().axis >= 0; }

// -----------------------------------------------------------------------------
// テクスチャ / 保存
// -----------------------------------------------------------------------------
std::string ToTextureName(const std::string& resourcePath) {
	namespace fs = std::filesystem;
	const std::string prefix = "Resources/Textures/";
	std::string path = resourcePath;
	std::replace(path.begin(), path.end(), '\\', '/');
	if (path.compare(0, prefix.size(), prefix) == 0) {
		return path.substr(prefix.size());
	}
	const fs::path relative = fs::path(path).lexically_relative("Resources/Textures");
	return relative.empty() ? path : relative.generic_string();
}

bool IsImagePath(const std::string& path) {
	std::string ext = std::filesystem::path(path).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".bmp" || ext == ".tga";
}

bool IsModified(const EditorObject& e) {
	const Baselines& baselines = BaselineStore();
	auto it = baselines.values.find(e.name);
	return it != baselines.values.end() && !SameData(it->second, Snapshot(e));
}

bool SaveObject(const EditorObject& e) {
	if (!HasSavableValues(e)) {
		return false;
	}
	const SceneSaveData::ObjectData data = Snapshot(e);
	if (!SceneSaveData::Store(e.name, data)) {
		return false;
	}
	BaselineStore().values[e.name] = data; // 保存した状態を「変更なし」の基準にする
	return true;
}

#ifdef USE_IMGUI
namespace {
// テクスチャ欄: 今のテクスチャ・ドロップ先・Project で選択中の画像を使う・元に戻す
void DrawTextureField(EditorObject& e) {
	static std::string lastError;
	static std::string lastErrorObject;
	const std::string current = e.getTexture();
	ImGui::TextWrapped("%s", current.empty() ? "(なし)" : current.c_str());

	auto apply = [&](const std::string& resourcePath) {
		const std::string name = ToTextureName(resourcePath);
		if (e.setTexture(name)) {
			lastError.clear();
		} else {
			lastError = "読み込めませんでした: " + name;
			lastErrorObject = e.name;
		}
	};

	// Project ウィンドウから画像をドラッグ＆ドロップ
	ImGui::Button("ここに画像をドロップ", ImVec2(-FLT_MIN, 0.0f));
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
			const std::string path = static_cast<const char*>(payload->Data);
			if (IsImagePath(path)) {
				apply(path);
			} else {
				lastError = "画像ファイルではありません: " + path;
				lastErrorObject = e.name;
			}
		}
		ImGui::EndDragDropTarget();
	}
	// Project ウィンドウでクリックして選んだ画像を使う
	const std::string selectedPath = GetProjectSelectedPath();
	const bool canUseSelected = IsImagePath(selectedPath);
	if (!canUseSelected) ImGui::BeginDisabled();
	if (ImGui::Button("Project で選択中の画像を使う")) {
		apply(selectedPath);
	}
	if (!canUseSelected) ImGui::EndDisabled();
	if (e.clearTexture) {
		ImGui::SameLine();
		if (ImGui::Button("元に戻す##texture")) {
			e.clearTexture();
			lastError.clear();
		}
	}
	if (!lastError.empty() && lastErrorObject == e.name) {
		ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.4f, 1.0f), "%s", lastError.c_str());
	}
}

// 保存欄: 変更あり表示・この状態を保存・保存を削除・変更を元に戻す
void DrawSaveSection(EditorObject& e) {
	ImGui::SeparatorText("保存");
	const SceneSaveData::ObjectData* saved = SceneSaveData::Find(e.name);
	if (IsModified(e)) {
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "* 実行中に変更あり（保存しないと次回は戻ります）");
	} else if (saved) {
		ImGui::TextDisabled("保存済み");
	}
	if (ImGui::Button("この状態を保存")) {
		SaveObject(e);
	}
	if (IsModified(e)) {
		ImGui::SameLine();
		if (ImGui::Button("変更を元に戻す")) {
			auto it = BaselineStore().values.find(e.name);
			if (it != BaselineStore().values.end()) {
				ApplyData(e, it->second);
			}
		}
	}
	if (saved) {
		ImGui::SameLine();
		if (ImGui::Button("保存を削除")) {
			SceneSaveData::Remove(e.name);
		}
		if (!SceneSaveData::WasApplied(e.name)) {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "このシーンはまだ保存値を読み込んでいません。");
			ImGui::TextWrapped("シーンの Initialize で SceneSaveData::Apply(\"%s\", obj) を呼ぶと、次回の起動から反映されます。", e.name.c_str());
		}
	}
	ImGui::TextDisabled("%s", SceneSaveData::GetFilePath().c_str());
}
} // namespace
#endif // USE_IMGUI

// -----------------------------------------------------------------------------
// Inspector
// -----------------------------------------------------------------------------
void DrawObjectInspector(EditorObject& e) {
#ifdef USE_IMGUI
	ImGui::Text("%s", e.name.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", e.type);

	const bool hasTransform = e.getPosition || e.getRotation || e.getScale;
	if (hasTransform && ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (e.getPosition && e.setPosition) {
			Math::Vector3 p = e.getPosition();
			if (e.is3D) {
				if (ImGui::DragFloat3("Position", &p.x, 0.05f)) e.setPosition(p);
			} else {
				if (ImGui::DragFloat2("Position", &p.x, 1.0f)) e.setPosition(p);
			}
		}
		if (e.getRotation && e.setRotation) {
			Math::Vector3 r = e.getRotation();
			if (e.is3D) {
				// 見やすいよう度で表示する（中身はラジアン）
				Math::Vector3 deg{r.x * kRadToDeg, r.y * kRadToDeg, r.z * kRadToDeg};
				if (ImGui::DragFloat3("Rotation (deg)", &deg.x, 0.5f)) e.setRotation({deg.x * kDegToRad, deg.y * kDegToRad, deg.z * kDegToRad});
			} else {
				float deg = r.z * kRadToDeg;
				if (ImGui::DragFloat("Rotation (deg)", &deg, 0.5f)) e.setRotation({0.0f, 0.0f, deg * kDegToRad});
			}
		}
		if (e.getScale && e.setScale) {
			Math::Vector3 s = e.getScale();
			if (e.is3D) {
				if (ImGui::DragFloat3("Scale", &s.x, 0.01f)) e.setScale(s);
			} else if (std::strcmp(e.type, "Text") == 0) {
				if (ImGui::DragFloat("Scale", &s.x, 0.01f, 0.0f, 100.0f)) e.setScale(s);
			} else {
				if (ImGui::DragFloat2("Size", &s.x, 1.0f)) e.setScale(s);
			}
		}
	}
	if (e.getColor && e.setColor && ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen)) {
		Math::Vector4 c = e.getColor();
		if (ImGui::ColorEdit4("Color", &c.x)) e.setColor(c);
	}
	if (e.getTexture && e.setTexture && ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
		DrawTextureField(e);
	}
	if (HasSavableValues(e)) {
		DrawSaveSection(e);
	}
	if (e.drawExtra) {
		ImGui::Spacing();
		e.drawExtra();
	}
#else
	(void)e;
#endif // USE_IMGUI
}

// -----------------------------------------------------------------------------
// Gizmo（移動用の矢印）
// -----------------------------------------------------------------------------
void DrawGizmo(EditorObject& e, const Camera& camera, float minX, float minY, float maxX, float maxY) {
#ifdef USE_IMGUI
	GizmoDrag& drag = Drag();
	// ボタンを離したらドラッグ終了（対象が見えなくなっていても必ず戻す）
	if (drag.axis >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		drag.axis = -1;
	}
	if (!e.is3D || !e.getPosition || !e.setPosition) {
		if (drag.objectName == e.name) drag.axis = -1;
		return;
	}
	const float width = maxX - minX;
	const float height = maxY - minY;
	if (width <= 0.0f || height <= 0.0f) {
		return;
	}

	// ワールド座標 → 画面座標（カメラの後ろなら false）
	const Math::Matrix4x4& vp = camera.GetViewProjectionMatrix();
	auto project = [&](const Math::Vector3& p, ImVec2& out) {
		const float x = p.x * vp.m[0][0] + p.y * vp.m[1][0] + p.z * vp.m[2][0] + vp.m[3][0];
		const float y = p.x * vp.m[0][1] + p.y * vp.m[1][1] + p.z * vp.m[2][1] + vp.m[3][1];
		const float w = p.x * vp.m[0][3] + p.y * vp.m[1][3] + p.z * vp.m[2][3] + vp.m[3][3];
		if (w <= 1e-4f) {
			return false;
		}
		out = ImVec2(minX + (x / w * 0.5f + 0.5f) * width, minY + (0.5f - y / w * 0.5f) * height);
		return true;
	};

	const Math::Vector3 origin = e.getPosition();
	ImVec2 originScreen;
	if (!project(origin, originScreen)) {
		return;
	}
	// 画面上でだいたい同じ大きさに見えるよう、カメラとの距離に比例した長さにする
	const Math::Vector3 cam = camera.GetTranslate();
	const float dx = origin.x - cam.x, dy = origin.y - cam.y, dz = origin.z - cam.z;
	const float axisLength = (std::max)(std::sqrt(dx * dx + dy * dy + dz * dz) * 0.15f, 0.01f);

	const Math::Vector3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	const ImU32 colors[3] = {IM_COL32(230, 70, 70, 255), IM_COL32(110, 210, 80, 255), IM_COL32(70, 130, 240, 255)};
	const ImU32 hotColor = IM_COL32(255, 220, 60, 255);

	const ImGuiIO& io = ImGui::GetIO();
	const ImVec2 mouse = io.MousePos;
	// マウスがゲーム画面の上にあるか（他のウィンドウが重なっていないか）
	const ImGuiWindow* hovered = GImGui->HoveredWindow;
	const bool mouseOverGame = mouse.x >= minX && mouse.x <= maxX && mouse.y >= minY && mouse.y <= maxY &&
	                           (hovered == nullptr || std::strcmp(hovered->RootWindow->Name, "Game") == 0);

	ImDrawList* dl = ImGui::GetForegroundDrawList();
	dl->PushClipRect(ImVec2(minX, minY), ImVec2(maxX, maxY), true);

	int hoveredAxis = -1;
	ImVec2 ends[3];
	bool visible[3] = {};
	for (int i = 0; i < 3; ++i) {
		const Math::Vector3 tip{origin.x + axes[i].x * axisLength, origin.y + axes[i].y * axisLength, origin.z + axes[i].z * axisLength};
		visible[i] = project(tip, ends[i]);
		if (!visible[i] || drag.axis >= 0) {
			continue;
		}
		// 線分とマウスの距離で当たり判定
		const float sx = ends[i].x - originScreen.x, sy = ends[i].y - originScreen.y;
		const float len2 = sx * sx + sy * sy;
		if (len2 < 1.0f || !mouseOverGame) {
			continue;
		}
		const float t = std::clamp(((mouse.x - originScreen.x) * sx + (mouse.y - originScreen.y) * sy) / len2, 0.0f, 1.0f);
		const float px = originScreen.x + sx * t - mouse.x, py = originScreen.y + sy * t - mouse.y;
		if (px * px + py * py < 8.0f * 8.0f) {
			hoveredAxis = i;
		}
	}

	// ドラッグ開始
	if (drag.axis < 0 && hoveredAxis >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		const float sx = ends[hoveredAxis].x - originScreen.x, sy = ends[hoveredAxis].y - originScreen.y;
		const float len = std::sqrt(sx * sx + sy * sy);
		drag.objectName = e.name;
		drag.axis = hoveredAxis;
		drag.startMouseX = mouse.x;
		drag.startMouseY = mouse.y;
		drag.startPosition = origin;
		drag.screenDirX = sx / len;
		drag.screenDirY = sy / len;
		drag.pixelsPerUnit = len / axisLength;
	}
	// ドラッグ中：マウスの移動量を「軸の画面上の向き」に投影して、ワールドの移動量にする
	if (drag.axis >= 0 && drag.objectName == e.name) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			const float moved = ((mouse.x - drag.startMouseX) * drag.screenDirX + (mouse.y - drag.startMouseY) * drag.screenDirY) / drag.pixelsPerUnit;
			const Math::Vector3& a = axes[drag.axis];
			e.setPosition({drag.startPosition.x + a.x * moved, drag.startPosition.y + a.y * moved, drag.startPosition.z + a.z * moved});
		} else {
			drag.axis = -1;
		}
	}

	// 描画（奥の軸から描かなくても見分けられるよう、太線＋先端の三角）
	for (int i = 0; i < 3; ++i) {
		if (!visible[i]) {
			continue;
		}
		const bool hot = (i == hoveredAxis) || (drag.axis == i && drag.objectName == e.name);
		const ImU32 col = hot ? hotColor : colors[i];
		dl->AddLine(originScreen, ends[i], col, hot ? 4.0f : 3.0f);
		const float sx = ends[i].x - originScreen.x, sy = ends[i].y - originScreen.y;
		const float len = std::sqrt(sx * sx + sy * sy);
		if (len > 1.0f) {
			const float ux = sx / len, uy = sy / len;
			const float head = 10.0f;
			dl->AddTriangleFilled(ImVec2(ends[i].x + ux * head, ends[i].y + uy * head), ImVec2(ends[i].x - uy * head * 0.5f, ends[i].y + ux * head * 0.5f),
			                      ImVec2(ends[i].x + uy * head * 0.5f, ends[i].y - ux * head * 0.5f), col);
		}
	}
	dl->AddCircleFilled(originScreen, 4.0f, IM_COL32(240, 240, 240, 255));
	dl->AddText(ImVec2(originScreen.x + 8.0f, originScreen.y + 6.0f), IM_COL32(240, 240, 240, 220), e.name.c_str());
	dl->PopClipRect();
#else
	(void)e; (void)camera; (void)minX; (void)minY; (void)maxX; (void)maxY;
#endif // USE_IMGUI
}

// -----------------------------------------------------------------------------
// Hierarchy（フォルダ付きの木）
// -----------------------------------------------------------------------------
#ifdef USE_IMGUI
namespace {
constexpr const char* kObjectPayload = "EDITOR_OBJECT";
constexpr const char* kFolderPayload = "EDITOR_FOLDER";

std::string LeafName(const std::string& path) {
	const size_t slash = path.rfind('/');
	return slash == std::string::npos ? path : path.substr(slash + 1);
}
std::string ParentOf(const std::string& path) {
	const size_t slash = path.rfind('/');
	return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

// 名前変更 / 新規フォルダの入力
struct FolderEdit {
	std::string target;  // 名前を変えるフォルダ（新規ならその親）
	bool isNew = false;
	char buffer[128] = {};
	bool open = false;
};
FolderEdit& Edit() {
	static FolderEdit edit;
	return edit;
}

void BeginFolderEdit(const std::string& target, bool isNew) {
	FolderEdit& ed = Edit();
	ed.target = target;
	ed.isNew = isNew;
	const std::string initial = isNew ? std::string("New Folder") : LeafName(target);
	std::snprintf(ed.buffer, sizeof(ed.buffer), "%s", initial.c_str());
	ed.open = true;
}

// オブジェクト / フォルダのドロップを受けて、folder に入れる
void AcceptDropInto(const std::string& folder) {
	if (!ImGui::BeginDragDropTarget()) {
		return;
	}
	if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(kObjectPayload)) {
		SceneSaveData::SetFolder(static_cast<const char*>(p->Data), folder);
	}
	if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(kFolderPayload)) {
		const std::string source = static_cast<const char*>(p->Data);
		const std::string dest = folder.empty() ? LeafName(source) : folder + "/" + LeafName(source);
		SceneSaveData::RenameFolder(source, dest);
	}
	ImGui::EndDragDropTarget();
}

void DrawObjectItem(EditorObject& object, std::string& selected, const std::function<void()>& onSelect) {
	ImGui::PushID(object.name.c_str());
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (selected == object.name) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	const bool modified = IsModified(object);
	const bool saved = SceneSaveData::Find(object.name) != nullptr;
	ImGui::TreeNodeEx("##object", flags, "%s %s%s%s", object.is3D ? "[3D]" : "[2D]", object.name.c_str(), modified ? " *" : "", saved && !modified ? "  (保存済)" : "");
	if (ImGui::IsItemClicked()) {
		selected = object.name;
		if (onSelect) onSelect();
	}
	if (modified && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
		ImGui::SetTooltip("実行中に変更あり。右クリック →「この状態を保存」で次回も使えます");
	}
	if (ImGui::BeginDragDropSource()) {
		ImGui::SetDragDropPayload(kObjectPayload, object.name.c_str(), object.name.size() + 1);
		ImGui::Text("%s", object.name.c_str());
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginPopupContextItem("##objectMenu")) {
		if (ImGui::MenuItem("この状態を保存", nullptr, false, HasSavableValues(object))) {
			SaveObject(object);
		}
		if (ImGui::MenuItem("保存を削除", nullptr, false, saved)) {
			SceneSaveData::Remove(object.name);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("フォルダから出す", nullptr, false, !SceneSaveData::GetFolder(object.name).empty())) {
			SceneSaveData::SetFolder(object.name, "");
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

// フォルダ1つ分（中のフォルダ → 中のオブジェクトの順）
void DrawFolder(const std::string& folder, std::string& selected, const std::function<bool(const std::string&)>& passFilter, const std::function<void()>& onSelect) {
	ImGui::PushID(folder.c_str());
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.82f, 0.45f, 1.0f));
	const bool open = ImGui::TreeNodeEx("##folder", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen, "[Folder] %s", LeafName(folder).c_str());
	ImGui::PopStyleColor();
	if (ImGui::BeginDragDropSource()) {
		ImGui::SetDragDropPayload(kFolderPayload, folder.c_str(), folder.size() + 1);
		ImGui::Text("[Folder] %s", LeafName(folder).c_str());
		ImGui::EndDragDropSource();
	}
	AcceptDropInto(folder);
	if (ImGui::BeginPopupContextItem("##folderMenu")) {
		if (ImGui::MenuItem("この中に新しいフォルダ")) BeginFolderEdit(folder, true);
		if (ImGui::MenuItem("名前を変更")) BeginFolderEdit(folder, false);
		if (ImGui::MenuItem("削除（中身は1つ上へ）")) SceneSaveData::RemoveFolder(folder);
		ImGui::EndPopup();
	}
	if (open) {
		// 直下のフォルダ
		for (const std::string& child : std::vector<std::string>(SceneSaveData::GetFolders())) {
			if (ParentOf(child) == folder) {
				DrawFolder(child, selected, passFilter, onSelect);
			}
		}
		// 直下のオブジェクト
		for (EditorObject& object : Objects()) {
			if (SceneSaveData::GetFolder(object.name) == folder && passFilter(object.name)) {
				DrawObjectItem(object, selected, onSelect);
			}
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

// 名前変更 / 新規フォルダのポップアップ
void DrawFolderEditPopup() {
	FolderEdit& ed = Edit();
	if (ed.open) {
		ImGui::OpenPopup("##folderEdit");
		ed.open = false;
	}
	if (ImGui::BeginPopup("##folderEdit")) {
		ImGui::Text(ed.isNew ? "新しいフォルダ" : "フォルダの名前を変更");
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		const bool enter = ImGui::InputText("##name", ed.buffer, sizeof(ed.buffer), ImGuiInputTextFlags_EnterReturnsTrue);
		std::string name = ed.buffer;
		std::replace(name.begin(), name.end(), '/', '_'); // / はフォルダの区切りなので使わない
		const bool valid = !name.empty();
		if (!valid) ImGui::BeginDisabled();
		if ((ImGui::Button("OK") || enter) && valid) {
			if (ed.isNew) {
				std::string path = ed.target.empty() ? name : ed.target + "/" + name;
				// 同じ名前があれば番号を付ける
				const auto& folders = SceneSaveData::GetFolders();
				std::string unique = path;
				for (int n = 2; std::find(folders.begin(), folders.end(), unique) != folders.end(); ++n) {
					unique = path + " " + std::to_string(n);
				}
				SceneSaveData::AddFolder(unique);
			} else {
				const std::string parent = ParentOf(ed.target);
				SceneSaveData::RenameFolder(ed.target, parent.empty() ? name : parent + "/" + name);
			}
			ImGui::CloseCurrentPopup();
		}
		if (!valid) ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("キャンセル")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}
} // namespace
#endif // USE_IMGUI

void AcceptDropToRoot() {
#ifdef USE_IMGUI
	AcceptDropInto("");
#endif // USE_IMGUI
}

void DrawObjectTree(std::string& selected, const std::function<bool(const std::string&)>& passFilter, const std::function<void()>& onSelect) {
#ifdef USE_IMGUI
	// 上段: フォルダ作成 / 変更をまとめて保存
	if (ImGui::SmallButton("+ フォルダ")) {
		BeginFolderEdit("", true);
	}
	int modifiedCount = 0;
	for (const EditorObject& object : Objects()) {
		if (IsModified(object)) ++modifiedCount;
	}
	ImGui::SameLine();
	if (modifiedCount == 0) ImGui::BeginDisabled();
	char label[64];
	std::snprintf(label, sizeof(label), "変更をすべて保存 (%d)", modifiedCount);
	if (ImGui::SmallButton(label)) {
		for (const EditorObject& object : Objects()) {
			if (IsModified(object)) SaveObject(object);
		}
	}
	if (modifiedCount == 0) ImGui::EndDisabled();

	// フォルダ（一番上の階層）→ フォルダに入っていないオブジェクト
	for (const std::string& folder : std::vector<std::string>(SceneSaveData::GetFolders())) {
		if (ParentOf(folder).empty()) {
			DrawFolder(folder, selected, passFilter, onSelect);
		}
	}
	const auto& folders = SceneSaveData::GetFolders();
	for (EditorObject& object : Objects()) {
		const std::string folder = SceneSaveData::GetFolder(object.name);
		// 入っているフォルダが消されていた場合も、ここ（一番上）に出す
		const bool inExistingFolder = !folder.empty() && std::find(folders.begin(), folders.end(), folder) != folders.end();
		if (!inExistingFolder && passFilter(object.name)) {
			DrawObjectItem(object, selected, onSelect);
		}
	}
	if (Objects().empty()) {
		ImGui::TextDisabled("(オブジェクト未登録: シーンの ImGuiDraw で Editor::Inspect)");
	}
	DrawFolderEditPopup();
#else
	(void)selected; (void)passFilter; (void)onSelect;
#endif // USE_IMGUI
}

} // namespace TuboEngine::Editor
