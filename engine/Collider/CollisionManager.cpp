#include "CollisionManager.h"
#include "Collider.h"

/// -------------------------------------------------------------
///							初期化処理
/// -------------------------------------------------------------
void CollisionManager::Initialize() {
	isCollider_ = true;
	
}

/// -------------------------------------------------------------
///							更新処理
/// -------------------------------------------------------------
void CollisionManager::Update() {
	// 非表示なら抜ける
	if (!isCollider_) {
		return;
	}

	// 更新処理
	for (Collider* collider : colliders_) {
		collider->Update();
	}
}

/// -------------------------------------------------------------
///							描画処理
/// -------------------------------------------------------------
void CollisionManager::Draw() {
	// 非表示なら抜ける
	if (!isCollider_) {
		return;
	}

	// 描画処理
	for (Collider* collider : colliders_) {
		if (isCollider_) {
			collider->Draw();
		}
	}
}

/// -------------------------------------------------------------
///							リセット処理
/// -------------------------------------------------------------
void CollisionManager::Reset() { colliders_.clear(); }

/// -------------------------------------------------------------
///				すべての当たり判定を確認する処理
/// -------------------------------------------------------------
void CollisionManager::CheckAllCollisions() {
	// リスト内のペアを総当たり
	std::list<Collider*>::iterator itrA = colliders_.begin();
	for (; itrA != colliders_.end(); ++itrA) {
		Collider* colliderA = *itrA;

		// イテレーターBは入れてータAの次の要素からまわす（重複判定を回避）
		std::list<Collider*>::iterator itrB = itrA;
		itrB++;

		for (; itrB != colliders_.end(); ++itrB) {
			Collider* colliderB = *itrB;

			// ペアの当たり判定
			CheckCollisionPair(colliderA, colliderB);
		}
	}
}

/// -------------------------------------------------------------
///						コライダー追加処理
/// -------------------------------------------------------------
void CollisionManager::AddCollider(Collider* other) { colliders_.push_back(other); }

void CollisionManager::RemoveCollider(Collider* other) {
	colliders_.remove(other);
	
}

/// -------------------------------------------------------------
///				コライダー２つの衝突判定と応答処理
/// -------------------------------------------------------------
void CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {
	// 球体同士の衝突判定
	if (CheckSphereCollision(colliderA, colliderB)) {
		// コライダーAの衝突時コールバックを呼び出す
		colliderA->OnCollision(colliderB);
		// コライダーBの衝突時コールバックを呼び出す
		colliderB->OnCollision(colliderA);

		// 当たっていたら
		colliderA->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // コライダーAを赤に染める
		colliderB->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // コライダーBを赤に染める
	} else {
		colliderA->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		colliderB->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	}
}

/// -------------------------------------------------------------
///						球体同士の衝突判定
/// -------------------------------------------------------------
bool CollisionManager::CheckSphereCollision(Collider* colliderA, Collider* colliderB) {
	// コライダーAの座標を取得
	TuboEngine::Math::Vector3 positionA = colliderA->GetCenterPosition();
	// コライダーBの座標を取得
	TuboEngine::Math::Vector3 positionB = colliderB->GetCenterPosition();
	// 座標の差分ベクトル
	TuboEngine::Math::Vector3 subtract = positionB - positionA;
	// AとBの距離を求める
	float distance = subtract.Length();
	// コライダーAとコライダーBの半径の加算
	float radiusSum = colliderA->GetRadius() + colliderB->GetRadius();

	return distance <= radiusSum;
}
