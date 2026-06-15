#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"
#include<memory>
#include "SkyBox/SkyBoxPSO.h"

namespace TuboEngine {

class SkyBoxCommon
{

public:
	// シングルトンインスタンス取得
	static SkyBoxCommon* GetInstance() {
		if (!instance) {
			instance = new SkyBoxCommon();
		}
		return instance;
	}

private:
	static SkyBoxCommon* instance; // シングルトンインスタンス
	SkyBoxCommon() = default; // コンストラクタ（外部から生成不可）
	~SkyBoxCommon() = default; // デストラクタ
	SkyBoxCommon(const SkyBoxCommon&) = delete; // コピーコンストラクタ禁止
	SkyBoxCommon& operator=(const SkyBoxCommon&) = delete; // 代入演算子禁止

public:
	/*------------------------------------------------------------
			関数
	------------------------------------------------------------*/

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	void Finalize();

	/// <summary>
    /// 共通描画設定
    /// </summary>
	void DrawSettingsCommon();
	
private:
	std::unique_ptr<SkyBoxPSO> pso = nullptr; // スカイボックスのパイプラインステートオブジェクト
};

} // namespace TuboEngine

