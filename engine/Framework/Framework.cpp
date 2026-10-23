#include "Framework.h"
#include"WinApp.h"
#include <dxcapi.h>
#include "TextManager.h"
#include "Object3d.h"

void TuboEngine::Framework::Initialize() {

	TuboEngine::WinApp::GetInstance()->Initialize();

	// DirectX共通部分

	TuboEngine::DirectXCommon::GetInstance()->Initialize();

	// リソースの有効性を確認
	if (!TuboEngine::DirectXCommon::GetInstance()->GetDevice() || !TuboEngine::DirectXCommon::GetInstance()->GetCommandList()) {
		throw std::runtime_error("DirectXリソースの初期化に失敗しました。");
	}

#ifdef USE_IMGUI

	// ImGuiの初期化

	TuboEngine::ImGuiManager::GetInstance()->Initialize();

#endif // USE_IMGUI

	// SRVマネージャーの初期化
	TuboEngine::SrvManager::GetInstance()->Initialize();
	// スプライト共通部分
	SpriteCommon::GetInstance()->Initialize();

	// オブジェクト3Dの共通部分
	Object3dCommon::GetInstance()->Initialize();

	// パーティクル共通部分
	ParticleCommon::GetInstance()->Initialize();

	// テクスチャマネージャーの初期化
	TuboEngine::TextureManager::GetInstance()->Initialize();

	// モデルマネージャーの初期化
	ModelManager::GetInstance()->initialize();

	// オーディオ共通部
	AudioCommon::GetInstance()->Initialize();

	// 入力初期化
	TuboEngine::Input::GetInstance()->Initialize(TuboEngine::WinApp::GetInstance()->GetHWND());

	// オフスクリーンレンダリングの初期化
	OffScreenRendering::GetInstance()->Initialize();

	// ラインマネージャーの初期化
	LineManager::GetInstance()->Initialize();

	std::string testDDSTextureHandle = "DDS/rostock_laage_airport_4k.dds";
	TuboEngine::TextureManager::GetInstance()->LoadTexture(testDDSTextureHandle);

	// TextManagerの初期化
	TuboEngine::TextManager::GetInstance()->Initialize();

	// シーンの登録と開始シーンの指定はゲーム側（Order::Initialize）で行う
	// （エンジンは「どのシーンが存在するか／どこから始めるか」を決めない）
}
void TuboEngine::Framework::Update() {
	// メッセージ処理
	if (TuboEngine::WinApp::GetInstance()->ProcessMessage()) {
		endRequest = true;
	}

	// 入力の更新
	TuboEngine::Input::GetInstance()->Update();

	// マウスカーソルの固定（入力の後に行う：Esc / クリックでの切り替えを今フレームの入力で判定するため）
	UpdateCursorLock();

	// シーンマネージャーの更新（デバッグ時はツールバーの一時停止/コマ送りに従う）
	bool updateScene = true;
#ifdef USE_IMGUI
	updateScene = TuboEngine::ImGuiManager::GetInstance()->ConsumeSceneUpdate();
#endif // USE_IMGUI
	// 経過時間の計測（一時停止中は DeltaTime が 0 になる）
	TuboEngine::Time::Update(updateScene);
	if (updateScene) {
		SceneManager::GetInstance()->Update();
	}

	TuboEngine::Camera* mainCamera = SceneManager::GetInstance()->GetMainCamera();

	if (mainCamera) {
		// ラインマネージャーのカメラ設定
		LineManager::GetInstance()->SetDefaultCamera(mainCamera);
		OffScreenRendering::GetInstance()->SetCamera(mainCamera);
	}

	// オフスクリーンレンダリングの更新
	OffScreenRendering::GetInstance()->Update();
	//
	LineManager::GetInstance()->Update();
}
void TuboEngine::Framework::Finalize() {
	// シーンを最初に終了する。シーンの Finalize / デストラクタは Input・TextureManager・
	// ParticleManager などを使うことがあるので、それらより先に片付ける
	// （後にすると解放済みのものを触る / GetInstance() で作り直されてリークする）
	SceneManager::GetInstance()->Finalize();

#ifdef USE_IMGUI
	TuboEngine::ImGuiManager::GetInstance()->Finalize();
#endif // USE_IMGUI

	TuboEngine::Input::GetInstance()->Finalize();

	// TextManagerの解放
	TuboEngine::TextManager::DestroyInstance();

	// パーティクルマネージャの明示解放（エミッター内のGPUリソースを先に解放する）
	TuboEngine::ParticleManager::GetInstance()->Finalize();

	TuboEngine::TextureManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	CloseHandle(TuboEngine::DirectXCommon::GetInstance()->GetFenceEvent());

	OffScreenRendering::GetInstance()->Finalize();
	TuboEngine::SrvManager::GetInstance()->Finalize();

	ParticleCommon::GetInstance()->Finalize();
	SpriteCommon::GetInstance()->Finalize();
  TuboEngine::SharedLightResourcesRelease();
	Object3dCommon::GetInstance()->Finalize();
	TuboEngine::SkyBoxCommon::GetInstance()->Finalize();
	LineManager::GetInstance()->Finalize();

	AudioCommon::GetInstance()->Finalize();
	TuboEngine::DirectXCommon::GetInstance()->Finalize();
	TuboEngine::WinApp::GetInstance()->Finalize();
}

void TuboEngine::Framework::UpdateCursorLock() {
	TuboEngine::WinApp* winApp = TuboEngine::WinApp::GetInstance();
	const HWND hwnd = winApp->GetHWND();
	bool active = (GetForegroundWindow() == hwnd);
	// 閉じ込める範囲（クライアント座標）。既定はウィンドウ全体
	RECT area{};
	::GetClientRect(hwnd, &area);

#ifdef USE_IMGUI
	TuboEngine::Input* input = TuboEngine::Input::GetInstance();
	TuboEngine::ImGuiManager* gui = TuboEngine::ImGuiManager::GetInstance();
	const ImGuiIO& io = ImGui::GetIO();
	const bool gameView = gui->IsGameViewActive();

	// Game ウィンドウ表示に切り替わった直後は、エディタを触れるよう固定を外しておく
	if (gameView && !cursorGameViewPrev_) {
		winApp->SetCursorCaptured(false);
	}
	cursorGameViewPrev_ = gameView;
	// 別のウィンドウに切り替えたら解除（戻ってきた時にいきなり閉じ込めない）
	if (!active) {
		winApp->SetCursorCaptured(false);
	}
	// Esc で一時解除（Unity の Play モードと同じ操作）
	if (!io.WantTextInput && input->TriggerKey(DIK_ESCAPE)) {
		winApp->SetCursorCaptured(false);
	}

	// Game ウィンドウ表示中は、その画像の範囲に閉じ込める
	bool hasArea = true;
	if (gameView) {
		hasArea = gui->GetGameViewRect(area);
	}

	// 解除中にゲーム画面をクリックしたら再び固定する
	POINT cursor{};
	::GetCursorPos(&cursor);
	::ScreenToClient(hwnd, &cursor);
	const bool cursorInArea = hasArea && ::PtInRect(&area, cursor);
	if (active && !winApp->IsCursorCaptured() && cursorInArea && !io.WantCaptureMouse && input->IsTriggerMouse(0)) {
		winApp->SetCursorCaptured(true);
	}

	// 従来表示（ゲーム画面の上に ImGui が重なる）では、ImGui を操作中は閉じ込めない
	active = active && hasArea && (gameView || !io.WantCaptureMouse);
#endif // USE_IMGUI

	winApp->UpdateCursor(active, area);
}

void TuboEngine::Framework::Run() {
	// 初期化
	Initialize();
	// メインループ
	while (true) {
		// 更新
		Update();

		// 終了リクエストがあったら
		if (IsEndRequest()) {
			// ループを抜ける
			break;
		}

		// 描画
		Draw();
	}
	// 終了処理
	Finalize();
}

void TuboEngine::Framework::FrameworkSwapChainPreDraw() {
	// 描画前処理
	TuboEngine::DirectXCommon::GetInstance()->PreDraw();
}

void TuboEngine::Framework::FrameworkSwapChainPostDraw() {
#ifdef USE_IMGUI
	// ImGuiの描画
	TuboEngine::ImGuiManager::GetInstance()->Draw();
#endif // USE_IMGUI

	OffScreenRendering::GetInstance()->TransitionRenderTextureToRenderTarget();
	// 描画
	TuboEngine::DirectXCommon::GetInstance()->PostDraw();
}

void TuboEngine::Framework::ImguiPreDraw() {
#ifdef USE_IMGUI
	// ImGuiの受付開始（メニューバー/ドックスペースは常に出す＝再表示の操作口を残す）
	TuboEngine::ImGuiManager::GetInstance()->Begin();

	// 「全ウィンドウ表示」がONの時だけ、シーン/各マネージャのデバッグUIを描画する。
	// OFFにすると下記をまるごとスキップ＝全デバッグウィンドウが消える。
	if (TuboEngine::ImGuiManager::GetInstance()->DebugWindowsVisible()) {
		SceneManager::GetInstance()->ImGuiDraw();
		OffScreenRendering::GetInstance()->DrawImGui();
	}
#endif // USE_IMGUI
}

void TuboEngine::Framework::ImguiPostDraw() {
#ifdef USE_IMGUI
	// 「全ウィンドウ表示」がONの時だけデバッグ用ウィンドウを描画する
	if (TuboEngine::ImGuiManager::GetInstance()->DebugWindowsVisible()) {
		if (*TuboEngine::ImGuiManager::GetInstance()->PanelPtr("Demo")) {
			ImGui::ShowDemoWindow(TuboEngine::ImGuiManager::GetInstance()->PanelPtr("Demo"));
		}
		// BlendMode変更
		if (TuboEngine::ImGuiManager::GetInstance()->BeginPanel("BlendNum")) {
			ImGui::Text("BlendMode");
			ImGui::Text("0: None");
			ImGui::Text("1: Normal");
			ImGui::Text("2: Add");
			ImGui::Text("3: Subtract");
			ImGui::Text("4: Multiply");
			ImGui::Text("5: Screen");
			ImGui::SliderInt("BlendNum", &objectBlendModeNum, 0, 5);
			ImGui::SliderInt("SpriteBlendNum", &spriteBlendModeNum, 0, 5);
		}
		TuboEngine::ImGuiManager::GetInstance()->EndPanel();
	}

	// 受付終了(Render)は常に呼ぶ（メニューバーを含むフレームを確定させるため）
	ImGuiManager::GetInstance()->End();
#endif // USE_IMGUI
}

void TuboEngine::Framework::FrameWorkRenderTargetPreDraw() {

	// ImGuiの受付開始
	OffScreenRendering::GetInstance()->PreDraw();

	TuboEngine::SrvManager::GetInstance()->PreDraw();
}

void TuboEngine::Framework::Object3dCommonDraw() {
	// オブジェクト3Dの描画
	TuboEngine::Object3dCommon::GetInstance()->DrawSettingsCommon(objectBlendModeNum);

	// 3Dオブジェクトの描画
	SceneManager::GetInstance()->Object3DDraw();
}

void TuboEngine::Framework::SpriteCommonDraw() {

	// スプライトの描画
	SpriteCommon::GetInstance()->DrawSettingsCommon(spriteBlendModeNum);
	SceneManager::GetInstance()->SpriteDraw();
}

void TuboEngine::Framework::ParticleCommonDraw() {
	// パーティクルの描画
	ParticleCommon::GetInstance()->DrawSettingsCommon();
	SceneManager::GetInstance()->ParticleDraw();
}

void TuboEngine::Framework::OffScreenRenderingDraw() {
	OffScreenRendering::GetInstance()->TransitionRenderTextureToShaderResource();

	OffScreenRendering::GetInstance()->TransitionDepthTo(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// オフスクリーンレンダリングの描画
	OffScreenRendering::GetInstance()->Draw();
}
