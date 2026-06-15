#include "Camera.h"
#include"WinApp.h"
#include"Matrix.h"

namespace TuboEngine {
Camera::Camera()

    : transform_({
          {0.0f, 0.0f, 0.0f},
          {0.0f, 0.0f, 0.0f},
          {1.0f, 1.0f, 1.0f}
}),
      fovY_(0.45f), aspect_(float(TuboEngine::WinApp::GetInstance()->GetClientWidth()) / float(TuboEngine::WinApp::GetInstance()->GetClientHeight())), nearClip_(0.1f), farClip_(100.0f),
      worldMatrix_(MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate)), viewMatrix_(Inverse(worldMatrix_)),
      projectionMatrix_(TuboEngine::Math::MakePerspectiveMatrix(fovY_, aspect_, nearClip_, farClip_)), viewProjectionMatrix_(Multiply(viewMatrix_, projectionMatrix_)) {}

void Camera::Update() {
	// ワールド行列の作成
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	// ビュー行列の作成
	viewMatrix_ = Inverse(worldMatrix_);
	// プロジェクション行列の作成
	projectionMatrix_ = TuboEngine::Math::MakePerspectiveMatrix(fovY_, aspect_, nearClip_, farClip_);
	// ビュープロジェクション行列の作成
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}
} // namespace TuboEngine