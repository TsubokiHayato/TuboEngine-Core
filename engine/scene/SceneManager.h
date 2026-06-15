#pragma once
#include<memory>
#include<functional>
#include<unordered_map>
#include<map>
#include<string>
#include"IScene.h"
class SceneManager
{
public:
	/// <summary>
	/// シングルトンインスタンス取得
	/// </summary>
	static SceneManager* GetInstance() {
		
		if (!instance) {
			instance = new SceneManager();
		}
		return instance;
	}

private:
 bool forceReload_ = false;
	// コンストラクタ・デストラクタ・コピー禁止
	static SceneManager* instance ;
	SceneManager() = default;
	~SceneManager() = default;
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;


public:
	// シーン生成関数の型（ゲーム側が登録する）
	using SceneFactory = std::function<std::unique_ptr<IScene>()>;

	// シーン番号→生成関数を登録する（ゲーム側が起動時に呼ぶ）
	// debugName を渡すと ImGui のシーン選択に表示される
	void RegisterScene(int sceneNo, SceneFactory factory, const std::string& debugName = "");

	//初期化（開始シーン番号はゲーム側が指定する）
	void Initialize(int startSceneNo);
	//更新
	void Update();
	//終了処理
	void Finalize();
	//オブジェクト3D描画
	void Object3DDraw();
	//スプライト描画
	void SpriteDraw();
	//ImGui描画
	void ImGuiDraw();
	// パーティクル描画
	void ParticleDraw();

	// シーン切り替え
	void ChangeScene(int sceneNo);

	// MainCamera取得
	TuboEngine::Camera* GetMainCamera() const {
		if (currentScene) {
			return currentScene->GetMainCamera();
		}
		return nullptr;
	}

private:
	// 登録テーブルからシーンを生成する（未登録なら nullptr）
	std::unique_ptr<IScene> CreateScene(int sceneNo);

	// シーン番号→生成関数 / デバッグ表示名
	std::unordered_map<int, SceneFactory> factories_;
	std::map<int, std::string> debugNames_; // 番号順に並ぶよう map を使用（ImGui表示用）

	//現在のシーン
	std::unique_ptr<IScene> currentScene = nullptr;
	//前のシーン
	std::unique_ptr<IScene> prevScene = nullptr;
	//現在のシーン番号
	int currentSceneNo = 0;
	//前のシーン番号
	int prevSceneNo = 0;
};

