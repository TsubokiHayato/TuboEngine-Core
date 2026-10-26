#include "SceneSaveData.h"
#include "Animator.h"
#include "JsonManager.h"
#include "Object3d.h"
#include "SceneManager.h"
#include "Sprite.h"
#include "TextObject.h"
#include "TextureManager.h"
#include <algorithm>
#include <map>
#include <set>

// =============================================================================
//  ファイルの形式（Resources/SceneData/<シーン名>.json）
//  {
//    "objects": {
//      "player": { "position": [0, 1, 0], "rotation": [0, 3.14, 0], "scale": [1, 1, 1],
//                  "color": [1, 1, 1, 1], "texture": "player.png" }      ← 保存した項目だけ
//    },
//    "hierarchy": {
//      "folders": ["Enemies", "Enemies/Boss"],
//      "assign": { "enemy01": "Enemies" }
//    }
//  }
// =============================================================================

namespace TuboEngine {

using Json = JsonManager::Json;

namespace {

struct State {
	std::string sceneName;        // 読み込んでいるシーン
	uint32_t sceneGeneration = 0; // 読み込んだ時のシーンの番号（リロードの検出）
	bool loaded = false;
	std::map<std::string, SceneSaveData::ObjectData> objects;
	std::vector<std::string> folders;
	std::map<std::string, std::string> assign;
	std::set<std::string> applied; // このシーンで Apply された名前
};

State& S() {
	static State state;
	return state;
}

std::string FilePathFor(const std::string& sceneName) { return "Resources/SceneData/" + sceneName + ".json"; }

// 今のシーンのデータを用意する（シーンが変わったら読み直す）
State& Current() {
	State& s = S();
	SceneManager* sm = SceneManager::GetInstance();
	const std::string scene = sm->GetCurrentSceneName();
	const uint32_t generation = sm->GetSceneGeneration();
	if (s.loaded && s.sceneName == scene) {
		if (s.sceneGeneration != generation) {
			s.applied.clear(); // 同じシーンを作り直した → Apply し直されるはず
			s.sceneGeneration = generation;
		}
		return s;
	}
	s = State{};
	s.sceneName = scene;
	s.sceneGeneration = generation;
	s.loaded = true;

	Json root;
	if (!JsonManager::Load(FilePathFor(scene), root, /*logIfMissing*/ false) || !root.is_object()) {
		return s;
	}
	if (root.contains("objects") && root["objects"].is_object()) {
		for (const auto& [name, jo] : root["objects"].items()) {
			SceneSaveData::ObjectData d;
			Math::Vector3 v3{};
			Math::Vector4 v4{};
			std::string tex;
			if (JsonManager::Read(jo, "position", v3)) d.position = v3;
			if (JsonManager::Read(jo, "rotation", v3)) d.rotation = v3;
			if (JsonManager::Read(jo, "scale", v3)) d.scale = v3;
			if (JsonManager::Read(jo, "color", v4)) d.color = v4;
			if (JsonManager::Read(jo, "texture", tex)) d.texture = tex;
			s.objects[name] = d;
		}
	}
	if (root.contains("hierarchy") && root["hierarchy"].is_object()) {
		const Json& h = root["hierarchy"];
		JsonManager::Read(h, "folders", s.folders);
		if (h.contains("assign") && h["assign"].is_object()) {
			for (const auto& [name, folder] : h["assign"].items()) {
				if (folder.is_string()) s.assign[name] = folder.get<std::string>();
			}
		}
	}
	return s;
}

bool WriteFile(const State& s) {
	Json root;
	root["objects"] = Json::object();
	for (const auto& [name, d] : s.objects) {
		Json jo = Json::object();
		if (d.position) jo["position"] = *d.position;
		if (d.rotation) jo["rotation"] = *d.rotation;
		if (d.scale) jo["scale"] = *d.scale;
		if (d.color) jo["color"] = *d.color;
		if (d.texture) jo["texture"] = *d.texture;
		root["objects"][name] = jo;
	}
	root["hierarchy"]["folders"] = s.folders;
	root["hierarchy"]["assign"] = Json::object();
	for (const auto& [name, folder] : s.assign) {
		root["hierarchy"]["assign"][name] = folder;
	}
	return JsonManager::Save(FilePathFor(s.sceneName), root);
}

// テクスチャを読み込めたか（読めないテクスチャを設定すると描画で落ちる）
bool LoadTextureSafe(const std::string& name) {
	TextureManager::GetInstance()->LoadTexture(name);
	return TextureManager::GetInstance()->HasTexture(name);
}

const SceneSaveData::ObjectData* FindAndMark(const std::string& name) {
	State& s = Current();
	auto it = s.objects.find(name);
	s.applied.insert(name);
	return it != s.objects.end() ? &it->second : nullptr;
}

bool StartsWithFolder(const std::string& path, const std::string& folder) {
	return path == folder || (path.size() > folder.size() && path.compare(0, folder.size(), folder) == 0 && path[folder.size()] == '/');
}

std::string ParentFolder(const std::string& path) {
	const size_t slash = path.rfind('/');
	return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

} // namespace

// -----------------------------------------------------------------------------
// Apply
// -----------------------------------------------------------------------------
bool SceneSaveData::Apply(const std::string& name, Object3d* o) {
	const ObjectData* d = o ? FindAndMark(name) : nullptr;
	if (!d) return false;
	if (d->position) o->SetPosition(*d->position);
	if (d->rotation) o->SetRotation(*d->rotation);
	if (d->scale) o->SetScale(*d->scale);
	if (d->color) o->SetModelColor(*d->color);
	if (d->texture) o->SetTexture(*d->texture);
	return true;
}

bool SceneSaveData::Apply(const std::string& name, Animator* a) {
	const ObjectData* d = a ? FindAndMark(name) : nullptr;
	if (!d) return false;
	if (d->position) a->SetPosition(*d->position);
	if (d->rotation) a->SetRotation(*d->rotation);
	if (d->scale) a->SetScale(*d->scale);
	if (d->color) a->SetModelColor(*d->color);
	if (d->texture) a->GetObject3d()->SetTexture(*d->texture);
	return true;
}

bool SceneSaveData::Apply(const std::string& name, Sprite* sp) {
	const ObjectData* d = sp ? FindAndMark(name) : nullptr;
	if (!d) return false;
	if (d->position) sp->SetPosition({d->position->x, d->position->y});
	if (d->rotation) sp->SetRotation(d->rotation->z);
	if (d->scale) sp->SetSize({d->scale->x, d->scale->y});
	if (d->color) sp->SetColor(*d->color);
	if (d->texture && LoadTextureSafe(*d->texture)) sp->SetTexture(*d->texture);
	return true;
}

bool SceneSaveData::Apply(const std::string& name, TextObject* t) {
	const ObjectData* d = t ? FindAndMark(name) : nullptr;
	if (!d) return false;
	if (d->position) t->SetPosition({d->position->x, d->position->y});
	if (d->scale) t->SetScale(d->scale->x);
	if (d->color) t->SetColor(*d->color);
	return true;
}

// -----------------------------------------------------------------------------
// 保存
// -----------------------------------------------------------------------------
const SceneSaveData::ObjectData* SceneSaveData::Find(const std::string& name) {
	State& s = Current();
	auto it = s.objects.find(name);
	return it != s.objects.end() ? &it->second : nullptr;
}

bool SceneSaveData::Store(const std::string& name, const ObjectData& data) {
	State& s = Current();
	s.objects[name] = data;
	return WriteFile(s);
}

bool SceneSaveData::Remove(const std::string& name) {
	State& s = Current();
	if (s.objects.erase(name) == 0) return false;
	return WriteFile(s);
}

bool SceneSaveData::WasApplied(const std::string& name) { return Current().applied.contains(name); }

std::string SceneSaveData::GetFilePath() { return FilePathFor(Current().sceneName); }

// -----------------------------------------------------------------------------
// フォルダ
// -----------------------------------------------------------------------------
const std::vector<std::string>& SceneSaveData::GetFolders() { return Current().folders; }

std::string SceneSaveData::GetFolder(const std::string& objectName) {
	State& s = Current();
	auto it = s.assign.find(objectName);
	return it != s.assign.end() ? it->second : std::string();
}

void SceneSaveData::SetFolder(const std::string& objectName, const std::string& folder) {
	State& s = Current();
	if (folder.empty()) {
		if (s.assign.erase(objectName) == 0) return;
	} else {
		AddFolder(folder); // 無ければ作る（作った時は中で保存済み）
		if (s.assign[objectName] == folder) return;
		s.assign[objectName] = folder;
	}
	WriteFile(s);
}

bool SceneSaveData::AddFolder(const std::string& path) {
	State& s = Current();
	if (path.empty() || std::find(s.folders.begin(), s.folders.end(), path) != s.folders.end()) {
		return false;
	}
	// 親フォルダも作る（"A/B/C" なら "A", "A/B" も）
	for (size_t pos = path.find('/'); pos != std::string::npos; pos = path.find('/', pos + 1)) {
		const std::string parent = path.substr(0, pos);
		if (std::find(s.folders.begin(), s.folders.end(), parent) == s.folders.end()) {
			s.folders.push_back(parent);
		}
	}
	s.folders.push_back(path);
	std::sort(s.folders.begin(), s.folders.end());
	WriteFile(s);
	return true;
}

bool SceneSaveData::RenameFolder(const std::string& from, const std::string& to) {
	State& s = Current();
	if (from.empty() || to.empty() || from == to || StartsWithFolder(to, from)) {
		return false; // 自分の中へは移せない
	}
	if (std::find(s.folders.begin(), s.folders.end(), to) != s.folders.end()) {
		return false; // 同じ名前が既にある
	}
	for (std::string& f : s.folders) {
		if (StartsWithFolder(f, from)) f = to + f.substr(from.size());
	}
	for (auto& [name, folder] : s.assign) {
		if (StartsWithFolder(folder, from)) folder = to + folder.substr(from.size());
	}
	// 移動先の親フォルダが無ければ作る
	const std::string parent = ParentFolder(to);
	if (!parent.empty() && std::find(s.folders.begin(), s.folders.end(), parent) == s.folders.end()) {
		s.folders.push_back(parent);
	}
	std::sort(s.folders.begin(), s.folders.end());
	s.folders.erase(std::unique(s.folders.begin(), s.folders.end()), s.folders.end());
	WriteFile(s);
	return true;
}

void SceneSaveData::RemoveFolder(const std::string& path) {
	State& s = Current();
	const std::string parent = ParentFolder(path);
	std::erase_if(s.folders, [&](const std::string& f) { return StartsWithFolder(f, path); });
	for (auto it = s.assign.begin(); it != s.assign.end();) {
		if (StartsWithFolder(it->second, path)) {
			if (parent.empty()) {
				it = s.assign.erase(it);
				continue;
			}
			it->second = parent;
		}
		++it;
	}
	WriteFile(s);
}

} // namespace TuboEngine
