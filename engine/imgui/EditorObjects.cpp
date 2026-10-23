#include "EditorObjects.h"
#include "Animator.h"
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"
#include "TextObject.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h" // ホバー中のウィンドウ判定
#endif // USE_IMGUI
#include <algorithm>
#include <cmath>
#include <cstring>

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

} // namespace TuboEngine::Editor
