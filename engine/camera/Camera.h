#pragma once
#include"Transform.h"
#include"Matrix4x4.h"
class WinApp;

namespace TuboEngine {
class Camera {
public:
	/// <summary>
	///	コンストラクタ
	/// </summary>
	Camera();

	/// <summary>
	///	更新処理
	/// </summary>
	void Update();

	// Setter
	void SetTranslate(const TuboEngine::Math::Vector3& translate) { transform_.translate = translate; }
	void setRotation(const TuboEngine::Math::Vector3& rotation) { transform_.rotate = rotation; }
	void setScale(const TuboEngine::Math::Vector3& scale) { transform_.scale = scale; }
	void setFovY(float fovX) { fovY_ = fovX; }
	void setAspect(float aspect) { aspect_ = aspect; }
	void setNearClip(float nearClip) { nearClip_ = nearClip; }
	void setFarClip(float farClip) { farClip_ = farClip; }

	// Getter
	const TuboEngine::Math::Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	const TuboEngine::Math::Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	const TuboEngine::Math::Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
	const TuboEngine::Math::Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }
	const TuboEngine::Math::Vector3& GetTranslate() const { return transform_.translate; }
	const TuboEngine::Math::Vector3& GetRotation() const { return transform_.rotate; }
	const TuboEngine::Math::Vector3& GetScale() const { return transform_.scale; }

private:
	Transform transform_;                     // 座標変換
	TuboEngine::Math::Matrix4x4 worldMatrix_; // ワールド行列
	TuboEngine::Math::Matrix4x4 viewMatrix_;  // ビュー行列

	TuboEngine::Math::Matrix4x4 projectionMatrix_;     // プロジェクション行列
	TuboEngine::Math::Matrix4x4 viewProjectionMatrix_; // ビュープロジェクション行列

	float fovY_ = 0.0f;     // 水平方向視野角
	float aspect_ = 0.0f;   // アスペクト比
	float nearClip_ = 0.0f; // ニアクリップ距離
	float farClip_ = 0.0f;  // ファークリップ距離
};

} // namespace TuboEngine