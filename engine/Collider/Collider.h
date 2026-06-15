#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include <cstdint>



/// -------------------------------------------------------------
///					　当たり判定クラス
/// -------------------------------------------------------------
class Collider {
public: /// ---------- 純粋仮想関数 ---------- ///
	// 仮想デストラクタ
	virtual ~Collider() = default;

	// 衝突時に呼ばれる仮想関数
	virtual void OnCollision([[maybe_unused]] Collider* other) {}

	// 中心座標を取得する純粋仮想関数
	virtual TuboEngine::Math::Vector3 GetCenterPosition() const = 0;

public: /// ---------- デバッグ用メンバ関数 ---------- ///
	// 初期化処理
	void Initialize();

	// 更新処理
	void Update();

	// 描画処理
	void Draw();

public: /// ---------- 設定 ---------- ///
	// 識別IDを取得
	uint32_t GetTypeID() const { return typeID_; }

	// 半径を取得
	float GetRadius() const { return radius_; }

public: /// ---------- 取得 ---------- ///
	// 識別IDを設定
	void SetTypeID(uint32_t typeID) { typeID_ = typeID; }

	// 半径を設定
	void SetRadius(float radius) { radius_ = radius; }

	// 色を設定
	void SetColor(const TuboEngine::Math::Vector4& color) { defaultColor = color; }

private: /// ---------- メンバ変数 ---------- ///
	// 衝突半径
	float radius_ = 1.0f;

	// 識別ID
	uint32_t typeID_ = 0u;

	TuboEngine::Math::Vector4 defaultColor = {1.0f, 1.0f, 1.0f, 1.0f};
};

