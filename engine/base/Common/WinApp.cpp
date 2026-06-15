#include "WinApp.h"

#include"externals/imgui/imgui.h"

#pragma comment(lib,"winmm.lib")
//システムタイマーの分解能を上げる
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam,LRESULT lparam);
//ウィンドウプロシージャ
LRESULT CALLBACK CallWindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam) {
#ifdef USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
#endif // USE_IMGUI
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

namespace TuboEngine {

WinApp* WinApp::instance = nullptr; // シングルトンインスタンス
void WinApp::Finalize()
{	
	CloseWindow(hwnd);
	CoUninitialize();
	delete instance;
	instance = nullptr;
}

void WinApp::Initialize()
{

	//システムタイマーの分解能を上げる
	timeBeginPeriod(1);

    HRESULT	hr=CoInitializeEx(0, COINIT_MULTITHREADED);
	//ウィンドウプロシージャ
	wc.lpfnWndProc = CallWindowProc;
	//ウィンドウクラス名
	wc.lpszClassName = L"CG2WindowClass";
	//インタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//ウィンドウクラスを登録する
	RegisterClass(&wc);


	wrc = { 0,0,kClientWidth,kClientHeight };
	//クライアント領域をもとに実際サイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);


	//ウィンドウの生成
	hwnd = CreateWindow(
		wc.lpszClassName,//利用するクラス名
		L"LE4B_16_ツボキ_ハヤト",//タイトルバーの文字no
		WS_OVERLAPPEDWINDOW,//よく見るウィンドウスタイル
		CW_USEDEFAULT,//表示X座標(windowにまかせる）
		CW_USEDEFAULT,//表示Y座標(windowOSにまかせる）
		wrc.right - wrc.left,//ウィンドウ横幅
		wrc.bottom - wrc.top,//ウィンドウ縦幅
		nullptr,//親ウィンドウハンドル
		nullptr,//めにゅーハンドル
		wc.hInstance,//インスタンスハンドル
		nullptr);//オプション

	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	

}

bool WinApp::ProcessMessage()
{
	MSG msg{};
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (msg.message == WM_QUIT) {
		return true;
	}
	return false;
}

void WinApp::ToggleCursorClip(bool clip) {
	if (clip) {
		RECT clientRect;
		::GetClientRect(hwnd, &clientRect);
		POINT topLeft = {clientRect.left, clientRect.top};
		POINT bottomRight = {clientRect.right, clientRect.bottom};
		::ClientToScreen(hwnd, &topLeft);
		::ClientToScreen(hwnd, &bottomRight);
		
		RECT clipRect = {topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
		::ClipCursor(&clipRect);
	} else {
		::ClipCursor(nullptr);
	}
}

} // namespace TuboEngine
