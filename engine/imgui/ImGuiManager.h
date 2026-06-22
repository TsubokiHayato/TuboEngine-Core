#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"
#include <map>
#include <string>
#include <vector>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#endif // USE_IMGUI
namespace TuboEngine {
class ImGuiManager {
public:
	/// <summary>
	/// シングルトンインスタンス取得
	/// </summary>
	static ImGuiManager* GetInstance() {
		if (!instance) {
			instance = new ImGuiManager();
		}
		return instance;
	}

private:
	// コンストラクタ・デストラクタ・コピー禁止
	static ImGuiManager* instance;
	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// ImGuiの解放
	/// </summary>
	void Finalize();

	/// <summary>
	/// ImGui受付開始
	/// </summary>
	void Begin();

	/// <summary>
	/// ImGui受付終了
	///	</summary>
	void End();

	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void Draw();

	/// <summary>
	/// パネル(ウィンドウ)の表示フラグへのポインタを取得する（無ければ表示=trueで登録）。
	/// 使い方: ImGui::Begin("名前", ImGuiManager::GetInstance()->PanelPtr("名前"));
	/// こうすると Window メニューのチェックで表示/非表示でき、[X]ボタンでも閉じられる。
	/// </summary>
	bool* PanelPtr(const char* name);

	/// <summary>
	/// 全デバッグウィンドウの一括表示フラグ（Window メニューの「全ウィンドウ表示」）。
	/// これが false の間は、シーン/各マネージャの ImGui 描画をまるごとスキップする。
	/// </summary>
	bool& DebugWindowsVisible() { return debugWindowsVisible_; }

private:
	// パネル名 → 表示中か。Window メニューに列挙され、各 Begin に渡して表示制御する。
	std::map<std::string, bool> panelOpen_;
	// 全デバッグウィンドウの一括表示/非表示
	bool debugWindowsVisible_ = true;

#ifdef USE_IMGUI
	// SRVディスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
#endif // USE_IMGUI
};

} // namespace TuboEngine