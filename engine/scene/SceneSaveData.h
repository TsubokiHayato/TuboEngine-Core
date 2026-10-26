#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include <optional>
#include <string>
#include <vector>

// =============================================================================
//  SceneSaveData ── 実行中にエディタで変えた値を、シーンごとに保存・反映する
//
//  ・保存する（Debug）: Inspector の「この状態を保存」/ Hierarchy の右クリック →「保存」
//    → Resources/SceneData/<シーン名>.json に、そのオブジェクトの位置・回転・拡縮・色・テクスチャが入る
//  ・反映する（Release でも動く）: シーンの Initialize で、オブジェクトを作った後に1行呼ぶ
//
//      player_->Initialize("player/player.obj");
//      TuboEngine::SceneSaveData::Apply("player", player_.get());   // 保存が無ければ何もしない
//
//    名前は Editor::Inspect に渡した名前と同じにする。
//  ・Hierarchy のフォルダ分けも同じファイルに保存される（エディタで変えるとすぐ保存）。
// =============================================================================

namespace TuboEngine {

class Object3d;
class Animator;
class Sprite;
class TextObject;

class SceneSaveData {
public:
	/// <summary>
	/// オブジェクト1個分の保存値（保存しなかった項目は空）。
	/// </summary>
	struct ObjectData {
		std::optional<Math::Vector3> position;
		std::optional<Math::Vector3> rotation; // ラジアン
		std::optional<Math::Vector3> scale;
		std::optional<Math::Vector4> color;
		std::optional<std::string> texture;    // Resources/Textures からのパス
	};

	// --- ゲーム側（Release でも動く）---

	/// <summary>
	/// 今のシーンに保存されている値をオブジェクトに入れる。保存が無ければ false で何もしない。
	/// </summary>
	static bool Apply(const std::string& name, Object3d* object);
	static bool Apply(const std::string& name, Animator* animator);
	static bool Apply(const std::string& name, Sprite* sprite);
	static bool Apply(const std::string& name, TextObject* text);

	// --- エディタ用 ---

	/// <summary>
	/// 今のシーンの保存値（無ければ nullptr）。
	/// </summary>
	static const ObjectData* Find(const std::string& name);
	/// <summary>
	/// 値を保存してファイルに書く（同じ名前は上書き）。
	/// </summary>
	static bool Store(const std::string& name, const ObjectData& data);
	/// <summary>
	/// 保存を消してファイルに書く。
	/// </summary>
	static bool Remove(const std::string& name);
	/// <summary>
	/// このシーンで Apply が呼ばれたか（保存しても反映されない時の注意表示用）。
	/// </summary>
	static bool WasApplied(const std::string& name);
	/// <summary>
	/// 今のシーンの保存ファイルのパス。
	/// </summary>
	static std::string GetFilePath();

	// --- Hierarchy のフォルダ（"Enemies/Boss" のように / 区切り）---
	static const std::vector<std::string>& GetFolders();
	/// <summary>
	/// オブジェクトが入っているフォルダ（入っていなければ空）。
	/// </summary>
	static std::string GetFolder(const std::string& objectName);
	/// <summary>
	/// オブジェクトをフォルダに入れる（空ならフォルダから出す）。すぐ保存する。
	/// </summary>
	static void SetFolder(const std::string& objectName, const std::string& folder);
	/// <summary>
	/// フォルダを作る（親フォルダも無ければ作る）。すぐ保存する。既にあれば false。
	/// </summary>
	static bool AddFolder(const std::string& path);
	/// <summary>
	/// フォルダの名前・場所を変える（中のフォルダとオブジェクトも一緒に）。すぐ保存する。
	/// </summary>
	static bool RenameFolder(const std::string& from, const std::string& to);
	/// <summary>
	/// フォルダを消す（中のオブジェクトは1つ上のフォルダへ）。すぐ保存する。
	/// </summary>
	static void RemoveFolder(const std::string& path);
};

} // namespace TuboEngine
