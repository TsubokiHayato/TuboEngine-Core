#pragma once

#include<cstdint>
#include<windows.h>

namespace TuboEngine {

//! アプリケーションウィンドウ管理クラス（シングルトン）
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

	// マウスカーソルをウィンドウ内に制限する
	void ToggleCursorClip(bool clip);


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

};

} // namespace TuboEngine
