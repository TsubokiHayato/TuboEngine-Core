#pragma once

// =============================================================================
//  Timeline ウィンドウ（Blender のタイムライン / ドープシートのようなキーフレーム編集）
//
//  1. Hierarchy でオブジェクトを選ぶ（シーンが Editor::Inspect で登録した物）
//  2. 時間を合わせ、Inspector や Gizmo で位置などを動かして「キーを打つ (I)」
//  3. 別の時間で同じことを繰り返す → 再生ボタンで動きを確認
//  4. Save で JSON に保存 → ゲーム側は KeyframeClip / KeyframePlayer で再生（KeyframeClip.h）
//
//  ・◆ をドラッグで時間を変更（Shift を押しながらで 1/60 秒単位に吸着しない）
//  ・◆ を右クリック → 削除 / 補間（イージング）の変更。Delete キーでも削除
//  ・目盛りの上をクリック / ドラッグで再生位置を移動
// =============================================================================
namespace TuboEngine::Editor {

struct EditorObject;

/// <summary>
/// Timeline ウィンドウを描く。target は Hierarchy で選択中のオブジェクト（無ければ nullptr）。
/// </summary>
void DrawTimelineWindow(bool* open, EditorObject* target);

} // namespace TuboEngine::Editor
