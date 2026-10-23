#pragma once

#include<cstdint>
#include<windows.h>

namespace TuboEngine {

/// <summary>
/// アプリケーションウィンドウ管理クラス（シングルトン）。
/// </summary>
class WinApp
{
public:

  
    // シングルトンインスタンス取得
    static WinApp* GetInstance() {
		if (!instance) {
			instance = new WinApp();
		}
		return instance;
	}

private:
	static WinApp* instance; // シングルトンインスタンス
    // コンストラクタ（外部から生成不可）
    WinApp() = default;
    // デストラクタ
    ~WinApp() = default;
    // コピーコンストラクタ禁止
    WinApp(const WinApp&) = delete;
    // 代入演算子禁止
    WinApp& operator=(const WinApp&) = delete;


public:
    // 終了処理
    void Finalize();

    // 初期化処理
    void Initialize();

    // ウィンドウハンドル取得
    HWND GetHWND() const { return hwnd; }

    // インスタンスハンドル取得
    HINSTANCE GetHInstance() const { return wc.hInstance; }

    // メッセージ構造体取得
    MSG GetMSG()const { return msg; }

   
    // メッセージ処理
    bool ProcessMessage();

	// マウスカーソルをウィンドウ内に制限する（その場で1回だけ。毎フレームの制御は UpdateCursor が行う）
	void ToggleCursorClip(bool clip);

	///------------------------------------------------
	/// マウスカーソルの固定
	///------------------------------------------------
	/// <summary>
	/// カーソルの固定方法。
	/// </summary>
	enum class CursorMode {
		Free,     // 固定しない（タイトル・メニュー向け）
		Confined, // ウィンドウ（Debug では Game ウィンドウ）内に閉じ込める ※既定
		Locked,   // 中央に固定して非表示（視点操作向け。移動量は Input::GetMouseMove で取る）
	};

	// 固定方法を設定・取得する。ゲーム側からいつでも切り替えてよい
	void SetCursorMode(CursorMode mode) { cursorMode_ = mode; }
	CursorMode GetCursorMode() const { return cursorMode_; }

	// 一時的に固定を外す / 戻す（モードはそのまま）。
	// Debug ビルドでは Esc で外れ、Game 画面をクリックすると戻る。
	void SetCursorCaptured(bool captured) { cursorCaptured_ = captured; }
	bool IsCursorCaptured() const { return cursorCaptured_; }

	/// <summary>
	/// モードに従ってカーソルを固定・解除する。Framework::Update から毎フレーム呼ぶ。
	/// （ClipCursor は Alt+Tab 等で OS に解除されるため、毎フレーム掛け直す）
	/// </summary>
	/// <param name="active">固定してよい状態か（ウィンドウが前面にある等）</param>
	/// <param name="clientArea">閉じ込める範囲（クライアント座標）</param>
	void UpdateCursor(bool active, const RECT& clientArea);


public:
	///------------------------------------------------
    /// Getter関数
	///------------------------------------------------
	// クライアント領域のサイズ取得
	RECT GetClientRect() const {
		return wrc;
	}
	// クライアント領域の幅取得
	int32_t GetClientWidth() const {
		return kClientWidth;
	}
	// クライアント領域の高さ取得
	int32_t GetClientHeight() const {
		return kClientHeight;
	}


private:
    // クライアント領域のサイズ（ウィンドウサイズを表す構造体）
	RECT wrc = {};

    // ウィンドウハンドル
    HWND hwnd = nullptr;

    // ウィンドウクラス情報
    WNDCLASS wc{};

    // メッセージ構造体
    MSG msg{};
    
    // クライアント領域の幅
    static const int32_t kClientWidth = 1280;
    // クライアント領域の高さ
    static const int32_t kClientHeight = 720;

	// カーソルの表示/非表示を切り替える（ShowCursor は回数カウント式なので、状態が変わる時だけ呼ぶ）
	void SetCursorVisible(bool visible);

	CursorMode cursorMode_ = CursorMode::Confined;
	bool cursorCaptured_ = true;
	bool cursorHidden_ = false;
};

} // namespace TuboEngine
