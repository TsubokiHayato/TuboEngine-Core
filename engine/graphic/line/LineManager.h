#pragma once
#include "DirectXCommon.h"
#include "Line.h"
#include "LineCommon.h"
#include "SrvManager.h"
#include <memory>
#include <vector>

namespace TuboEngine {

///----------------------------------------------------
// LineManagerクラス
///----------------------------------------------------

class LineManager {
	///----------------------------------------------------
	/// シングルトンパターン
	/// -----------------------------------------------------

public:
	static LineManager* GetInstance();

private:
	static LineManager* instance_;
	LineManager() = default;
	~LineManager() = default;
	LineManager(const LineManager&) = delete;
	LineManager& operator=(const LineManager&) = delete;

	///----------------------------------------------------
	/// メンバ変数
	///----------------------------------------------------
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// ImGuiによるグローバル描画
	/// </summary>
	void DrawImGui();

	/// <summary>
	/// ライン頂点情報クリア
	/// </summary>
	void ClearLines();

	/// <summary>
	/// ライン描画用頂点追加
	/// </summary>
	void DrawLine(const TuboEngine::Math::Vector3& start, const TuboEngine::Math::Vector3& end, const TuboEngine::Math::Vector4& color);

	/// <summary>
	/// グリッド描画
	/// </summary>
	/// <param name="size">グリッドのサイズ</param>
	/// <param name="split">分割数</param>
	/// <param name="color">グリッドの色</param>
	/// <param name="rotation">グリッドの回転</param>
	void DrawGrid(float size, int split, const TuboEngine::Math::Vector3& rotation = {0.0f, 0.0f, 0.0f}, const TuboEngine::Math::Vector4& color = {0.0f, 0.0f, 0.0f, 1.0f});

	/// <summary>
	/// 球描画
	/// </summary>
	/// <param name="center">球の中心座標</param>
	/// <param name="radius">球の半径</param>
	/// <param name="color">球の色</param>
	/// <param name="divisions">球の分割数</param>
	void DrawSphere(const TuboEngine::Math::Vector3& center, float radius, const TuboEngine::Math::Vector4& color, int divisions = 32);

	/// <summary>
	/// ライン系描画を完全に無効化（デバッグ用）
	/// </summary>
	void SetDisableAll(bool disable) { disableAll_ = disable; }
	bool GetDisableAll() const { return disableAll_; }

	///----------------------------------------------------
	/// ゲッター・セッター
	///----------------------------------------------------
public:
	TuboEngine::Camera* GetDefaultCamera() { return lineCommon_->GetDefaultCamera(); }
	void SetDefaultCamera(TuboEngine::Camera* camera) { lineCommon_->SetDefaultCamera(camera); }

	///-----------------------------------------------------
	/// メンバ変数
	///-----------------------------------------------------
private:
	std::unique_ptr<TuboEngine::Line> line_;
	std::unique_ptr<TuboEngine::LineCommon> lineCommon_;
	bool isDrawLine_ = true;
	bool isDrawGrid_ = true;
    bool disableAll_ = false;
	float gridSize_ = 16.0f;
	int gridDivisions_ = 2;
	TuboEngine::Math::Vector4 gridColor_ = {0.0f, 0.0f, 0.0f, 1.0f};
	bool isDrawSphere_ = true;
};
} // namespace TuboEngine