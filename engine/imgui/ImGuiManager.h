#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"
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

private:
#ifdef USE_IMGUI
	// SRVディスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
#endif // USE_IMGUI
};

} // namespace TuboEngine