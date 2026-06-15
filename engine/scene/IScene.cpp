#include "IScene.h"

// 既定の開始シーン番号。値の意味はゲーム側が持つ（旧 TITLE = 1 相当）。
// ※ 実際の開始シーンは Order::Initialize の SceneManager::Initialize(...) で制御する。
int IScene::sceneNo = 1;

IScene::~IScene()
{

}



