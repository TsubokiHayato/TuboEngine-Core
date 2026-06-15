#pragma once

#include"DirectXcommon.h"
#include"D3DResourceLeakChecker.h"
#include"SpriteCommon.h"
#include"Object3dCommon.h"
#include"TextureManager.h"
#include"ModelManager.h"
#include <SrvManager.h>
#include"AudioCommon.h"
#ifdef _DEBUG
#include"ImGuiManager.h"
#endif // DEBUG
#include"SceneManager.h"
#include"ParticleCommon.h"
#include"SkyBoxCommon.h"
#include"Input.h"
#include"OffScreenRendering.h"
#include"LineManager.h"
#include "ParticleManager.h"

class OffScreenRenderingPSO;

// フレームワーク基底クラス
// ゲームやアプリケーションのメインループや共通処理を管理します。

namespace TuboEngine {
class Framework {
public:
	/// <summary>
	/// 仮想デストラクタ
	/// </summary>
	virtual ~Framework() = default;

	/// <summary>
	/// 初期化処理
	/// 必要なリソースの生成や各種設定を行います。
	/// </summary>
	virtual void Initialize();

	/// <summary>
	/// 更新処理
	/// 毎フレーム呼ばれ、ゲームロジックや入力処理などを行います。
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 終了処理
	/// リソースの解放などを行います。
	/// </summary>
	virtual void Finalize();

	/// <summary>
	/// 描画処理
	/// 画面への描画を行います。
	/// </summary>
	virtual void Draw() = 0;

public:
	/// <summary>
	/// オフスクリーン描画前処理
	/// </summary>
	void FrameWorkRenderTargetPreDraw();

	/// <summary>
	/// オブジェクト3Dの描画
	/// </summary>
	void Object3dCommonDraw();

	/// <summary>
	/// スプライトの描画
	/// </summary>
	void SpriteCommonDraw();

	/// <summary>
	/// パーティクルの描画
	/// </summary>
	void ParticleCommonDraw();

	/// <summary>
	/// 描画前処理（スワップチェーン）
	/// </summary>
	void FrameworkSwapChainPreDraw();

	/// <summary>
	/// オフスクリーンレンダリングの描画
	/// </summary>
	void OffScreenRenderingDraw();

	/// <summary>
	/// ImGuiの受付開始
	/// </summary>
	void ImguiPreDraw();

	/// <summary>
	/// ImGuiの受付終了
	/// </summary>
	void ImguiPostDraw();

	/// <summary>
	/// 描画後処理（スワップチェーン）
	/// </summary>
	void FrameworkSwapChainPostDraw();

public:
	/// <summary>
	/// 終了リクエストがあったかどうか
	/// </summary>
	virtual bool IsEndRequest() { return endRequest; }

public:
	/// <summary>
	/// メインループの実行
	/// </summary>
	void Run();

	/// <summary>
	/// ブレンドモードの設定
	/// </summary>
	void SetBlendMode(int mode) { objectBlendModeNum = mode; }

	/// <summary>
	/// ブレンドモードの取得
	/// </summary>
	int GetBlendMode() { return objectBlendModeNum; }

protected:
	bool endRequest = false; // 終了リクエストフラグ

	int objectBlendModeNum = 1; // オブジェクトのブレンドモード
	int spriteBlendModeNum = 1; // スプライトのブレンドモード

	// 基盤システム
};
} // namespace TuboEngine