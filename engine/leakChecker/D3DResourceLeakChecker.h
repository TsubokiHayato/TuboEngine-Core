#pragma once
namespace TuboEngine {
/// <summary>
/// アプリ終了時に解放漏れの D3D12 リソースを検出・報告するクラス。
/// </summary>
class D3DResourceLeakChecker {
public:
	///< summary>
	/// デストラクタ
	///</summary>
	// リソースリークチェック
	~D3DResourceLeakChecker();
};

} // namespace TuboEngine