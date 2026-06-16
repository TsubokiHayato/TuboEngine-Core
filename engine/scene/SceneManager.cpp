#include "SceneManager.h"
#include"ImGuiManager.h"
// 具体シーンは include しない（どのシーンが存在するかはゲーム側が登録する）


SceneManager* SceneManager::instance = nullptr; // シングルトンインスタンス
void SceneManager::Initialize(int startSceneNo) {
    // 初期シーンを設定
    currentSceneNo = startSceneNo;
    prevSceneNo = -1;

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
	//前のシーン番号を設定
	prevSceneNo = currentSceneNo;
	//現在のシーン番号を取得
	currentSceneNo = currentScene->GetSceneNo();

    //前のシーン番号と現在のシーン番号が異なる場合
	if (prevSceneNo != currentSceneNo || forceReload_) {
		//現在のシーンがnullptrでない場合
		if (currentScene != nullptr) {
			//終了処理
			currentScene->Finalize();
		}

		//登録テーブルから新しいシーンを生成
		currentScene = CreateScene(currentSceneNo);
		if (currentScene) {
			currentScene->Initialize();
		}
       forceReload_ = false;
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
	ImGui::Begin("Scene");

	// 現在いるシーンを表示（登録名が無ければ番号のみ）
	auto currentIt = debugNames_.find(currentSceneNo);
	const char* currentName = (currentIt != debugNames_.end()) ? currentIt->second.c_str() : "(no name)";
	ImGui::Text("Current: %s (%d)", currentName, currentSceneNo);
	ImGui::Separator();

	for (const auto& [no, name] : debugNames_) {
		// 現在のシーンに対応するボタンをハイライト表示
		const bool isCurrent = (no == currentSceneNo);
		if (isCurrent) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		if (ImGui::Button(name.c_str())) {
			currentScene->SetSceneNo(no);
		}
		if (isCurrent) {
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();

	#endif // USE_IMGUI
}

void SceneManager::ChangeScene(int sceneNo) {
	//現在のシーンがnullptrでない場合
	if (currentScene) {
      //シーン番号を設定
		if (currentScene->GetSceneNo() == sceneNo) {
			forceReload_ = true;
		} else {
			currentScene->SetSceneNo(sceneNo);
		}
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
	// 登録テーブルを参照して生成（未登録なら nullptr）
	auto it = factories_.find(sceneNo);
	if (it != factories_.end() && it->second) {
		return it->second();
	}
	return nullptr;
}
