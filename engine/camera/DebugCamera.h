#pragma once
#include "Camera.h"
#include "Vector3.h"

namespace TuboEngine {

/// <summary>
/// デバッグ用フリーカメラ（シーンごとに1個ずつ持たせて使う）
/// Update() に各シーンのメインカメラを渡すと、有効中はそのカメラを「乗っ取って」
/// 自由に視点を動かせる（そのカメラを参照している全オブジェクトに反映される）。
///  - F2         : ON / OFF（Update 内でトグル）
///  - 右ドラッグ : 回転（ヨー / ピッチ）
///  - 中ドラッグ : 平行移動（パン）
///  - ホイール   : 前後ズーム（ドリー）
///  - WASD       : 前後左右移動 / Q,E : 上下移動
/// </summary>
class DebugCamera {
public:
	/// <summary>
	/// 対象カメラへ更新を適用する（有効時のみ乗っ取る）。
	/// シーンが自分のカメラを更新した後・描画前に毎フレーム呼ぶこと。
	/// F2 での ON/OFF 切り替えもこの中で行う。
	/// </summary>
	void Update(Camera* target);

	void SetActive(bool active);
	void ToggleActive() { SetActive(!active_); }
	bool IsActive() const { return active_; }

	/// <summary>ImGui デバッグ表示</summary>
	void DrawImGui();

private:
	Math::Vector3 translate_{0.0f, 0.0f, -15.0f}; // 位置
	Math::Vector3 rotate_{0.0f, 0.0f, 0.0f};      // 回転（x:ピッチ y:ヨー z:ロール）
	bool active_ = false;                          // 操作有効フラグ
	bool needSync_ = true;                         // 有効化直後に現在のカメラ姿勢へ合わせる

	// 速度パラメータ
	float moveSpeed_ = 0.1f;     // WASD / QE の移動速度
	float rotateSpeed_ = 0.003f; // マウス回転速度
	float panSpeed_ = 0.02f;     // 中ドラッグのパン速度
	float wheelSpeed_ = 0.01f;   // ホイールのズーム速度
};

} // namespace TuboEngine
