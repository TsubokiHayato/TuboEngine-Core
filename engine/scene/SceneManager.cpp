#include "SceneManager.h"
#include"ImGuiManager.h"
#include"Logger.h"   // 未登録シーン検知のログ出力
#include <cassert>
#include <format>
// 具体シーンは include しない（どのシーンが存在するかはゲーム側が登録する）


SceneManager* SceneManager::instance = nullptr; // シングルトンインスタンス
void SceneManager::Initialize(int startSceneNo) {
    // 初期シーンを設定
    currentSceneNo = startSceneNo;
    requestedSceneNo_ = -1;
    forceReload_ = false;

    // 登録テーブルから生成（どのシーンかはゲーム側が登録済み）
    currentScene = CreateScene(currentSceneNo);

    if (currentScene) {
        currentScene->Initialize();
    }
}

void SceneManager::Update() {
	//現在のシーンがnullptrでない場合
	if (currentScene == nullptr) {
		return;
	}

	// ChangeScene による遷移リクエストがあれば、ここで切り替える。
	//   requestedSceneNo_ >= 0 : 別シーンへ遷移
	//   forceReload_           : 同じシーンを作り直す
	if (requestedSceneNo_ >= 0 || forceReload_) {
		//現在のシーンを後始末
		currentScene->Finalize();

		// 別シーンへの遷移なら番号を更新（リロードは番号据え置き）
		if (requestedSceneNo_ >= 0) {
			currentSceneNo = requestedSceneNo_;
		}
		requestedSceneNo_ = -1;
		forceReload_ = false;

		//登録テーブルから新しいシーンを生成
		currentScene = CreateScene(currentSceneNo);
		if (currentScene) {
			currentScene->Initialize();
		}
	}
	//現在のシーンがnullptrでない場合
	if (currentScene) {
		//更新処理
		currentScene->Update();
	}
}

void SceneManager::Finalize() {
	//現在のシーンがnullptrでない場合
	if (currentScene) {
		//終了処理
		currentScene->Finalize();
	}
	delete instance;
	instance = nullptr;

}

void SceneManager::Object3DDraw() {
	//現在のシーンがnullptrでない場合
	if (currentScene) {
		//3Dオブジェクト描画
		currentScene->Object3DDraw();
	}
}

void SceneManager::SpriteDraw() {
	//現在のシーンがnullptrでない場合
	if (currentScene) {
		//スプライト描画
		currentScene->SpriteDraw();
	}
}

void SceneManager::ImGuiDraw() {

#ifdef USE_IMGUI
	//現在のシーンがnullptrでない場合
	if (currentScene) {
		//ImGui描画
		currentScene->ImGuiDraw();
	}

	//シーン選択ウィンドウ（登録された名前からデータ駆動で生成）
	if (TuboEngine::ImGuiManager::GetInstance()->BeginPanel("Scene")) {

	// 現在いるシーンを表示（登録名が無ければ番号のみ）
	auto currentIt = debugNames_.find(currentSceneNo);
	const char* currentName = (currentIt != debugNames_.end()) ? currentIt->second.c_str() : "(no name)";
	ImGui::Text("Current: %s (%d)", currentName, currentSceneNo);

	// パフォーマンス表示（ImGui が計測している平均フレームレート）
	const ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("FPS: %.1f  (%.2f ms/frame)", io.Framerate, (io.Framerate > 0.0f) ? 1000.0f / io.Framerate : 0.0f);

	// 現在のシーンを作り直す（同じ番号を渡すとリロードになる）
	if (ImGui::Button("Reload current scene")) {
		ChangeScene(currentSceneNo);
	}
	ImGui::Separator();

	for (const auto& [no, name] : debugNames_) {
		// 現在のシーンに対応するボタンをハイライト表示
		const bool isCurrent = (no == currentSceneNo);
		if (isCurrent) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		if (ImGui::Button(name.c_str())) {
			ChangeScene(no);
		}
		if (isCurrent) {
			ImGui::PopStyleColor();
		}
	}
	}
	TuboEngine::ImGuiManager::GetInstance()->EndPanel();

	#endif // USE_IMGUI
}

void SceneManager::DrawSceneMenuItems() {
#ifdef USE_IMGUI
	// 現在のシーン表示
	auto currentIt = debugNames_.find(currentSceneNo);
	const char* currentName = (currentIt != debugNames_.end()) ? currentIt->second.c_str() : "(no name)";
	ImGui::Text("Current: %s (%d)", currentName, currentSceneNo);
	ImGui::Separator();

	// 登録済みシーンを列挙。クリックでそのシーンへ遷移（現在シーンにチェックを付ける）。
	for (const auto& [no, name] : debugNames_) {
		const bool isCurrent = (no == currentSceneNo);
		if (ImGui::MenuItem(name.c_str(), nullptr, isCurrent)) {
			ChangeScene(no);
		}
	}

	ImGui::Separator();
	if (ImGui::MenuItem("Reload current scene")) {
		ChangeScene(currentSceneNo);
	}
#endif // USE_IMGUI
}

void SceneManager::ChangeScene(int sceneNo) {
	// 現在と同じ番号ならリロード、違えば遷移リクエストとして積む。
	// 実際の切り替えは次の Update() で行う。
	if (sceneNo == currentSceneNo) {
		forceReload_ = true;
	} else {
		requestedSceneNo_ = sceneNo;
	}
}

void SceneManager::ParticleDraw() {
	//現在のシーンがnullptrでない場合
	if (currentScene) {
		//パーティクル描画
		currentScene->ParticleDraw();
	}
}

void SceneManager::RegisterScene(int sceneNo, SceneFactory factory, const std::string& debugName) {
	// シーン番号→生成関数を登録
	factories_[sceneNo] = std::move(factory);
	if (!debugName.empty()) {
		debugNames_[sceneNo] = debugName;
	}
}

std::unique_ptr<IScene> SceneManager::CreateScene(int sceneNo) {
	// 登録テーブルを参照して生成
	auto it = factories_.find(sceneNo);
	if (it != factories_.end() && it->second) {
		return it->second();
	}
	// 未登録の番号。GameScenes.h に enum を足したのに SceneRegistration.cpp の
	// 登録を忘れている、というのが典型的な原因。黒画面で悩まないようここで知らせる。
	TuboEngine::Logger::Log(std::format(
		"[SceneManager] sceneNo={} is NOT registered. "
		"Add sm->RegisterScene(...) in application/scene/SceneRegistration.cpp.\n", sceneNo));
	assert(false && "ChangeScene: sceneNo is not registered (see SceneRegistration.cpp)");
	return nullptr;
}
