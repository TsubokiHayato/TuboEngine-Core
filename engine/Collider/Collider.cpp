#include "Collider.h"
#include"LineManager.h"

/// -------------------------------------------------------------
///						　	初期化処理
/// -------------------------------------------------------------
void Collider::Initialize() {}

/// -------------------------------------------------------------
///						　	 更新処理
/// -------------------------------------------------------------
void Collider::Update() {}

/// -------------------------------------------------------------
///						　	 描画処理
/// -------------------------------------------------------------
void Collider::Draw() {
	TuboEngine::Math::Vector3 center = GetCenterPosition();
	TuboEngine::LineManager::GetInstance()->DrawSphere(center, radius_, defaultColor, 16);
}
