#pragma once
#include"WinApp.h"
#include"DirectXCommon.h"
#include"Object3dCommon.h"
#include"SpriteCommon.h"
#include"ParticleCommon.h"
#include"ImGuiManager.h"
// シーン番号の具体的な意味（enum SCENE）はゲーム側が持つ:
//   application/scene/GameScenes.h
// エンジンは sceneNo を単なる int として扱う。

class IScene
{
protected:
	//シーン番号
	static int sceneNo;

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
	//シーン番号を取得
	int GetSceneNo() { return sceneNo; }
	//シーン番号を設定
	void SetSceneNo(int no) { sceneNo = no; }

};

