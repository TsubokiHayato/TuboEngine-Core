#include "LineManager.h"
#define _USE_MATH_DEFINES
#include "ImGuiManager.h"
#include "Matrix.h"
#include <math.h>

///----------------------------------------------------
/// シングルトンインスタンス初期化
///----------------------------------------------------
TuboEngine::LineManager* TuboEngine::LineManager::instance_ = nullptr;

///----------------------------------------------------
/// LineManagerインスタンス取得
///----------------------------------------------------
TuboEngine::LineManager* TuboEngine::LineManager::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = new LineManager();
	}
	return instance_;
}

///----------------------------------------------------
/// LineManager初期化処理
///----------------------------------------------------
void TuboEngine::LineManager::Initialize() {
	// Line描画共通処理クラス生成・初期化
	lineCommon_ = std::make_unique<TuboEngine::LineCommon>();
	lineCommon_->Initialize();
	// Line描画クラス生成・初期化
	line_ = std::make_unique<TuboEngine::Line>();
	line_->Initialize(lineCommon_.get());
}

///----------------------------------------------------
/// LineManager終了処理
///----------------------------------------------------
void TuboEngine::LineManager::Finalize() {
	delete instance_;
	instance_ = nullptr;
}

///----------------------------------------------------
/// LineManager更新処理
///----------------------------------------------------
void TuboEngine::LineManager::Update() {

	// Line描画クラス更新
	line_->Update();
}

///----------------------------------------------------
/// LineManager描画処理
///----------------------------------------------------
void TuboEngine::LineManager::Draw() {
   if (disableAll_) {
		line_->ClearLines();
		return;
	}
	// Line描画共通設定
	lineCommon_->DrawSettingsCommon();
	// Line描画
	line_->Draw();
	// ライン頂点情報クリア
	line_->ClearLines();
}

///----------------------------------------------------
/// ImGui描画処理
///----------------------------------------------------
void TuboEngine::LineManager::DrawImGui() {

#ifdef USE_IMGUI
	ImGui::Begin("LineManager");
  ImGui::Checkbox("Disable All", &disableAll_);
	ImGui::Separator();
	ImGui::Checkbox("Line", &isDrawLine_);
	ImGui::Separator();
	ImGui::Checkbox("Grid", &isDrawGrid_);
	ImGui::SliderFloat("GridSize", &gridSize_, 1.0f, 10000.0f);
	ImGui::SliderInt("Divisions", &gridDivisions_, 1, 512);
	ImGui::Checkbox("Sphere", &isDrawSphere_);
	ImGui::ColorEdit4("Color", &gridColor_.x);
	ImGui::End();
#endif //_USE_IMGUI
}

///----------------------------------------------------
/// ライン頂点情報クリア
///----------------------------------------------------
void TuboEngine::LineManager::ClearLines() { line_->ClearLines(); }

///----------------------------------------------------
/// ライン描画用頂点追加
///----------------------------------------------------
void TuboEngine::LineManager::DrawLine(const TuboEngine::Math::Vector3& start, const TuboEngine::Math::Vector3& end, const TuboEngine::Math::Vector4& color) {
 if (disableAll_ || !isDrawLine_) {
		return;
	}
	line_->DrawLine(start, end, color);
}

///----------------------------------------------------
/// グリッド描画
///----------------------------------------------------
void TuboEngine::LineManager::DrawGrid(float size, int split, const TuboEngine::Math::Vector3& rotation, const TuboEngine::Math::Vector4& color) {
    if (disableAll_ || !isDrawGrid_) {
		return;
	}
	// グリッドの中心座標
	TuboEngine::Math::Vector3 center = {0.0f, 0.0f, 0.0f};

	// 回転行列を生成
	TuboEngine::Math::Matrix4x4 rotMat = TuboEngine::Math::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, rotation, {0.0f, 0.0f, 0.0f});

	float half = size / 2.0f;
	float step = size / split;

	for (int i = 0; i <= split; ++i) {
		float pos = -half + i * step;

		// X軸方向の線
		TuboEngine::Math::Vector3 startX = {pos, 0.0f, -half};
		TuboEngine::Math::Vector3 endX = {pos, 0.0f, half};
		startX = TuboEngine::Math::TransformCoord(startX, rotMat);
		endX = TuboEngine::Math::TransformCoord(endX, rotMat);
		DrawLine(startX, endX, color);

		// Z軸方向の線
		TuboEngine::Math::Vector3 startZ = {-half, 0.0f, pos};
		TuboEngine::Math::Vector3 endZ = {half, 0.0f, pos};
		startZ = TuboEngine::Math::TransformCoord(startZ, rotMat);
		endZ = TuboEngine::Math::TransformCoord(endZ, rotMat);
		DrawLine(startZ, endZ, color);
	}
}

///----------------------------------------------------
/// 球描画
///----------------------------------------------------
void TuboEngine::LineManager::DrawSphere(const TuboEngine::Math::Vector3& center, float radius, const TuboEngine::Math::Vector4& color, int divisions) {
 if (disableAll_ || !isDrawSphere_ || divisions <= 0) {
		return;
	}
	float angleStep = 2.0f * static_cast<float>(M_PI) / divisions;
	for (int i = 0; i < divisions; ++i) {
		float angle1 = angleStep * i;
		float angle2 = angleStep * (i + 1);
		DrawLine(
		    TuboEngine::Math::Vector3(center.x + radius * cosf(angle1), center.y + radius * sinf(angle1), center.z),
		    TuboEngine::Math::Vector3(center.x + radius * cosf(angle2), center.y + radius * sinf(angle2), center.z), color);
		DrawLine(
		    TuboEngine::Math::Vector3(center.x + radius * cosf(angle1), center.y, center.z + radius * sinf(angle1)),
		    TuboEngine::Math::Vector3(center.x + radius * cosf(angle2), center.y, center.z + radius * sinf(angle2)), color);
		DrawLine(
		    TuboEngine::Math::Vector3(center.x, center.y + radius * cosf(angle1), center.z + radius * sinf(angle1)),
		    TuboEngine::Math::Vector3(center.x, center.y + radius * cosf(angle2), center.z + radius * sinf(angle2)), color);
	}
	for (int lat = 1; lat < divisions / 2; ++lat) {
		float latAngle1 = static_cast<float>(M_PI) * lat / (divisions / 2);
		float latAngle2 = static_cast<float>(M_PI) * (lat + 1) / (divisions / 2);
		float r1 = radius * sinf(latAngle1);
		float r2 = radius * sinf(latAngle2);
		float y1 = center.y + radius * cosf(latAngle1);
		float y2 = center.y + radius * cosf(latAngle2);
		for (int i = 0; i < divisions; ++i) {
			float angle1 = angleStep * i;
			float angle2 = angleStep * (i + 1);
			DrawLine(
			    TuboEngine::Math::Vector3(center.x + r1 * cosf(angle1), y1, center.z + r1 * sinf(angle1)), TuboEngine::Math::Vector3(center.x + r1 * cosf(angle2), y1, center.z + r1 * sinf(angle2)),
			    color);
			DrawLine(
			    TuboEngine::Math::Vector3(center.x + r2 * cosf(angle1), y2, center.z + r2 * sinf(angle1)), TuboEngine::Math::Vector3(center.x + r2 * cosf(angle2), y2, center.z + r2 * sinf(angle2)),
			    color);
		}
	}
	for (int lon = 0; lon < divisions; ++lon) {
		float lonAngle = angleStep * lon;
		for (int lat = 0; lat <= divisions / 2; ++lat) {
			float latAngle = static_cast<float>(M_PI) * lat / (divisions / 2);
			float nextLatAngle = static_cast<float>(M_PI) * (lat + 1) / (divisions / 2);
			float r1 = radius * sinf(latAngle);
			float r2 = radius * sinf(nextLatAngle);
			float y1 = center.y + radius * cosf(latAngle);
			float y2 = center.y + radius * cosf(nextLatAngle);
			DrawLine(
			    TuboEngine::Math::Vector3(center.x + r1 * cosf(lonAngle), y1, center.z + r1 * sinf(lonAngle)),
			    TuboEngine::Math::Vector3(center.x + r2 * cosf(lonAngle), y2, center.z + r2 * sinf(lonAngle)), color);
		}
	}
}
