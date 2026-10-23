#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"
#include <map>
#include <string>
#include <vector>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#endif // USE_IMGUI
namespace TuboEngine {
/// <summary>
/// ImGui の初期化・フレーム開始/終了・描画を管理するクラス。
/// </summary>
class ImGuiManager {
public:
	/// <summary>
	/// シングルトンインスタンス取得
	/// </summary>
	static ImGuiManager* GetInstance() {
		if (!instance) {
			instance = new ImGuiManager();
		}
		return instance;
	}

private:
	// コンストラクタ・デストラクタ・コピー禁止
	static ImGuiManager* instance;
	/// <summary>
	/// コンストラクタ。
	/// </summary>
	ImGuiManager() = default;
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~ImGuiManager() = default;
	/// <summary>
	/// コピー禁止。
	/// </summary>
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// ImGuiの解放
	/// </summary>
	void Finalize();

	/// <summary>
	/// ImGui受付開始
	/// </summary>
	void Begin();

	/// <summary>
	/// ImGui受付終了
	///	</summary>
	void End();

	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void Draw();

	/// <summary>
	/// パネル(ウィンドウ)の表示フラグへのポインタを取得する（無ければ表示=trueで登録）。
	/// 使い方: ImGui::Begin("名前", ImGuiManager::GetInstance()->PanelPtr("名前"));
	/// こうすると Window メニューのチェックで表示/非表示でき、[X]ボタンでも閉じられる。
	/// </summary>
	bool* PanelPtr(const char* name);

	/// <summary>
	/// 表示フラグ付きでウィンドウを開始する。戻り値が true の時だけ中身を描き、
	/// 必ず EndPanel() と対で使う。フラグが false なら ImGui::Begin を呼ばず中身もスキップ＝非表示。
	/// 使い方:  if (mgr->BeginPanel("名前")) { ...中身... }  mgr->EndPanel();
	/// </summary>
	bool BeginPanel(const char* name);
	void EndPanel();

	/// <summary>
	/// 全デバッグウィンドウの一括表示フラグ（Window メニューの「全ウィンドウ表示」）。
	/// これが false の間は、シーン/各マネージャの ImGui 描画をまるごとスキップする。
	/// </summary>
	bool& DebugWindowsVisible() { return debugWindowsVisible_; }

	/// <summary>
	/// ゲーム画面(ポストエフェクト後の最終結果)を描いたテクスチャを登録する。
	/// 登録すると Unity 風に「Game」ウィンドウの中へゲーム画面を表示できるようになる。
	/// </summary>
	void SetGameViewTexture(ID3D12Resource* texture);

	/// <summary>
	/// ゲーム画面を「Game」ウィンドウ内に表示するモードか。
	/// true の間、OffScreenRendering は最終結果をバックバッファではなくゲームビュー用テクスチャへ描く。
	/// </summary>
	bool IsGameViewActive() const;

	/// <summary>
	/// 前フレームに Game ウィンドウでゲーム画面を表示した範囲（クライアント座標）。
	/// Game ウィンドウが表示されていなければ false。マウスカーソルの固定範囲に使う。
	/// </summary>
	bool GetGameViewRect(RECT& outRect) const;

	/// <summary>
	/// このフレームでシーンを更新してよいか（ツールバーの一時停止/コマ送り用）。
	/// 一時停止中は false、コマ送り要求があった時だけ1回 true を返す。Framework::Update から呼ぶ。
	/// </summary>
	bool ConsumeSceneUpdate();

	/// <summary>
	/// 一時停止中か。
	/// </summary>
	bool IsPaused() const { return paused_; }

private:
#ifdef USE_IMGUI
	/// <summary>
	/// Unity 風の初期レイアウト（中央=Game / 右=デバッグパネル）を組み立てる。
	/// </summary>
	void BuildDefaultLayout(ImGuiID dockspaceId);

	/// <summary>
	/// 「Game」ウィンドウを描画し、ゲーム画面を縦横比を保って表示する。
	/// </summary>
	void DrawGameWindow();

	/// <summary>
	/// メニューバー中央の 再生/一時停止/コマ送り/リロード ボタン。
	/// </summary>
	void DrawToolbar();

	/// <summary>
	/// Hierarchy ウィンドウ：現在のシーンと、そのデバッグウィンドウ一覧を表示する。
	/// </summary>
	void DrawHierarchyWindow();

	/// <summary>
	/// Inspector ウィンドウ：選択中の項目とシーン/実行状態の情報を表示する。
	/// </summary>
	void DrawInspectorWindow();

	/// <summary>
	/// どこにもドッキングされていない(=ゲーム画面に重なる)デバッグウィンドウを、
	/// セッション中に初めて見つけた時だけ Inspector と同じ領域へドッキングする。
	/// </summary>
	void AutoDockFloatingWindows();
#endif // USE_IMGUI

private:
	// Unity 風レイアウト(ゲーム画面を Game ウィンドウ内に表示)を使うか
	bool gameViewEnabled_ = true;
	// 次フレームでレイアウトを初期状態へ戻す
	bool resetLayoutRequested_ = false;
	// ゲームビュー用テクスチャ
	ID3D12Resource* gameViewTexture_ = nullptr;
	// Game ウィンドウでの表示倍率（Inspector 表示用）
	float gameViewDisplayScale_ = 1.0f;
	// Game ウィンドウでゲーム画面を表示した範囲（クライアント座標）と、表示できたか
	RECT gameViewRect_ = {};
	bool gameViewRectValid_ = false;
	// 起動時に保存済みレイアウトを確認したか
	bool layoutChecked_ = false;
	// 一時停止中か / コマ送り要求
	bool paused_ = false;
	bool stepRequested_ = false;
	// エディタウィンドウの表示フラグ
	bool showHierarchy_ = true;
	bool showInspector_ = true;
	bool showProject_ = true;
	bool showConsole_ = true;
	// Hierarchy で選択中のウィンドウ名 / 前面に出す予定のウィンドウ名
	std::string selectedWindow_;
	std::string focusRequest_;
#ifdef USE_IMGUI
	// ゲームビュー用テクスチャの SRV（ImGui 用ヒープのスロット1）
	D3D12_GPU_DESCRIPTOR_HANDLE gameViewSrvGpu_{};
	// 自動ドッキング済みのウィンドウ（1セッションで1回だけ行う）
	std::vector<ImGuiID> autoDockedWindows_;
#endif // USE_IMGUI

	// パネル名 → 表示中か。Window メニューに列挙され、各 Begin に渡して表示制御する。
	std::map<std::string, bool> panelOpen_;
	// 全デバッグウィンドウの一括表示/非表示
	bool debugWindowsVisible_ = true;
	// BeginPanel/EndPanel の対応管理（true=ImGui::Beginを呼んだ＝Endが必要）
	std::vector<bool> panelBeganStack_;

#ifdef USE_IMGUI
	// SRVディスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
#endif // USE_IMGUI
};

} // namespace TuboEngine