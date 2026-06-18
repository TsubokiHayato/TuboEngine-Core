#pragma once
#include"WinApp.h"
#include"DirectXCommon.h"
#include"Object3dCommon.h"
#include"SpriteCommon.h"
#include"ParticleCommon.h"
#include"ImGuiManager.h"
// シーン番号の具体的な意味（enum SCENE）はゲーム側が持つ:
//   application/scene/GameScenes.h
//
// ▼ シーン遷移のしかた（重要）
//   別シーンへ移るときは SceneManager に依頼するだけ:
//       SceneManager::GetInstance()->ChangeScene(STAGE);
//   （現在シーンと同じ番号を渡すと「リロード」になる）
//   IScene 側に番号を持たせる作りは廃止したので、各シーンの Initialize に
//   「自分の番号を宣言するおまじない」は不要になった。

class IScene
{
public:

	//初期化
	virtual void Initialize() = 0;
	//更新
	virtual void Update() = 0;
	//終了処理
	virtual void Finalize() = 0;
	//オブジェクト3D描画
	virtual void Object3DDraw() = 0;
	//スプライト描画
	virtual void SpriteDraw() = 0;
	//ImGui描画
	virtual void ImGuiDraw() = 0;

	virtual void ParticleDraw() = 0;

	virtual TuboEngine::Camera* GetMainCamera() const = 0;

	//デストラクタ
	virtual ~IScene() = 0;

};

