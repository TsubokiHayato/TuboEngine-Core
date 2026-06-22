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

	// マウスカーソルの制限（ウィンドウがアクティブな時のみ）
	bool isForeground = (GetForegroundWindow() == TuboEngine::WinApp::GetInstance()->GetHWND());
	bool wantCaptureMouse = false;
#ifdef USE_IMGUI
	wantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
#endif
	// ウィンドウが前面にあり、かつ ImGui 操作中でない場合にのみ閉じ込める
	TuboEngine::WinApp::GetInstance()->ToggleCursorClip(isForeground && !wantCaptureMouse);
	// 入力の更新
	TuboEngine::Input::GetInstance()->Update();

	// シーンマネージャーの更新
	SceneManager::GetInstance()->Update();

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
	SceneManager::GetInstance()->Finalize();

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
		ImGui::ShowDemoWindow(TuboEngine::ImGuiManager::GetInstance()->PanelPtr("Demo"));
		// BlendMode変更
		ImGui::Begin("BlendNum", TuboEngine::ImGuiManager::GetInstance()->PanelPtr("BlendNum"));
		ImGui::Text("BlendMode");
		ImGui::Text("0: None");
		ImGui::Text("1: Normal");
		ImGui::Text("2: Add");
		ImGui::Text("3: Subtract");
		ImGui::Text("4: Multiply");
		ImGui::Text("5: Screen");
		ImGui::SliderInt("BlendNum", &objectBlendModeNum, 0, 5);
		ImGui::SliderInt("SpriteBlendNum", &spriteBlendModeNum, 0, 5);
		ImGui::End();
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
