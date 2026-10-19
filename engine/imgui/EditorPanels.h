#pragma once

// Unity 風エディタの補助ウィンドウ（Project / Console）。
// ImGuiManager がデバッグ時の Unity 風レイアウトで毎フレーム呼び出す。
namespace TuboEngine::Editor {

/// <summary>
/// Project ウィンドウ：Resources フォルダの中身をフォルダツリー＋アイコン一覧で表示する。
/// ファイルを右クリックでパスのコピー / エクスプローラーで表示ができる。
/// </summary>
void DrawProjectWindow(bool* open);

/// <summary>
/// Console ウィンドウ：Logger::Log で出したログを一覧表示する（検索・種類別の絞り込み付き）。
/// </summary>
void DrawConsoleWindow(bool* open);

} // namespace TuboEngine::Editor
