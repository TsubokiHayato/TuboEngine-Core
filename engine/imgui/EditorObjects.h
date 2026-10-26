#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include <functional>
#include <string>
#include <vector>

// =============================================================================
//  エディタのオブジェクト登録（Unity の Hierarchy / Inspector 用）
//
//  シーンの ImGuiDraw() で、毎フレーム「見せたいオブジェクト」を登録する:
//
//    void MyScene::ImGuiDraw() {
//    #ifdef USE_IMGUI
//        TuboEngine::Editor::Inspect("Player", player_.get());          // Object3d / Animator / Sprite / TextObject
//        TuboEngine::Editor::Inspect("Enemy", enemy_.get(), [this] {    // 独自の項目を足す時
//            ImGui::DragFloat("HP", &enemyHp_);
//        });
//    #endif
//    }
//
//  → Hierarchy に並び、クリックすると Inspector で位置・回転・拡縮・色を編集できる。
//    3D は Game 画面に移動用の矢印（Gizmo）が出る。Timeline でキーフレームアニメーションも作れる。
//
//  毎フレーム登録し直す方式なので、シーンが切り替わって消えたオブジェクトを触ることはない。
//  Release ビルドでは何もしない。
// =============================================================================

namespace TuboEngine {
class Object3d;
class Animator;
class Sprite;
class TextObject;
class Camera;
} // namespace TuboEngine

namespace TuboEngine::Editor {

/// <summary>
/// エディタに見せるオブジェクト1個分（値の読み書きは関数で行う）。
/// </summary>
struct EditorObject {
	std::string name;
	const char* type = "Custom";
	bool is3D = false; // true なら Gizmo を出す（ワールド座標）

	// 位置・回転（ラジアン）・拡縮。無い物は空のまま
	std::function<Math::Vector3()> getPosition;
	std::function<void(const Math::Vector3&)> setPosition;
	std::function<Math::Vector3()> getRotation;
	std::function<void(const Math::Vector3&)> setRotation;
	std::function<Math::Vector3()> getScale;
	std::function<void(const Math::Vector3&)> setScale;
	std::function<Math::Vector4()> getColor;
	std::function<void(const Math::Vector4&)> setColor;
	// テクスチャ（Resources/Textures からのパス）。setTexture は読み込めなければ false
	std::function<std::string()> getTexture;
	std::function<bool(const std::string&)> setTexture;
	std::function<void()> clearTexture; // 元のテクスチャに戻す（Object3d / Animator のみ）

	// Inspector の最後に足す独自の項目
	std::function<void()> drawExtra;
};

// --- 登録（シーンの ImGuiDraw から毎フレーム呼ぶ）---
void Inspect(const std::string& name, Object3d* object, std::function<void()> extra = nullptr);
void Inspect(const std::string& name, Animator* animator, std::function<void()> extra = nullptr);
void Inspect(const std::string& name, Sprite* sprite, std::function<void()> extra = nullptr);
void Inspect(const std::string& name, TextObject* text, std::function<void()> extra = nullptr);
/// <summary>
/// 位置などを持たない物（ゲームの設定値など）を Inspector に出す。
/// </summary>
void InspectCustom(const std::string& name, std::function<void()> draw);
/// <summary>
/// 自分で EditorObject を組み立てて登録する。
/// </summary>
void Register(EditorObject object);

// --- ImGuiManager 用 ---
/// <summary>
/// フレームの最初に呼ぶ（登録をリセット）。
/// </summary>
void BeginFrame();
/// <summary>
/// このフレームに登録されたオブジェクト。
/// </summary>
const std::vector<EditorObject>& GetObjects();
EditorObject* FindObject(const std::string& name);
/// <summary>
/// Inspector ウィンドウの中身（選択中のオブジェクトの項目）を描く。
/// </summary>
void DrawObjectInspector(EditorObject& object);
/// <summary>
/// Game 画面上に移動用の矢印を描き、ドラッグで動かす。imageMin / imageMax はゲーム画面の範囲（スクリーン座標）。
/// </summary>
void DrawGizmo(EditorObject& object, const Camera& camera, float imageMinX, float imageMinY, float imageMaxX, float imageMaxY);
/// <summary>
/// Gizmo をドラッグ中か（ドラッグ中はゲームへのクリックを気にする場合用）。
/// </summary>
bool IsGizmoDragging();

/// <summary>
/// Hierarchy にオブジェクトをフォルダ付きの木で描く（フォルダの作成・名前変更・削除、ドラッグでフォルダへ移動、
/// 右クリックで保存）。オブジェクトをクリックすると selected が変わり onSelect が呼ばれる。
/// </summary>
void DrawObjectTree(std::string& selected, const std::function<bool(const std::string&)>& passFilter, const std::function<void()>& onSelect);
/// <summary>
/// 直前に描いた項目（Hierarchy のシーン名）を「フォルダから出す」ドロップ先にする。
/// </summary>
void AcceptDropToRoot();

/// <summary>
/// 実行中に値が変わったか（シーン開始時 or 最後に保存した時の値と比べる）。
/// </summary>
bool IsModified(const EditorObject& object);
/// <summary>
/// 今の値を SceneSaveData に保存する（Resources/SceneData/<シーン名>.json）。
/// </summary>
bool SaveObject(const EditorObject& object);

/// <summary>
/// Resources からのパス（Project ウィンドウのパス）を TextureManager に渡す名前にする。
/// 例: "Resources/Textures/a.png" → "a.png"、"Resources/Models/x/b.png" → "../Models/x/b.png"
/// </summary>
std::string ToTextureName(const std::string& resourcePath);
/// <summary>
/// 画像ファイルか（拡張子で判定）。
/// </summary>
bool IsImagePath(const std::string& path);

} // namespace TuboEngine::Editor
