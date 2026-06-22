#include "ImGuiManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif // USE_IMGUI

#include <cassert>

#ifdef USE_IMGUI
namespace {
// Unity(ダークスキン)風のスタイルを適用する。
// 色・角丸・余白を Unity エディタに寄せる。docking 専用の色は IMGUI_HAS_DOCK で囲う。
void ApplyUnityStyle() {
	ImGuiStyle& style = ImGui::GetStyle();
	// 形状（Unityはほぼ角ばっている）
	style.WindowRounding    = 0.0f;
	style.ChildRounding     = 2.0f;
	style.FrameRounding     = 2.0f;
	style.PopupRounding     = 2.0f;
	style.GrabRounding      = 2.0f;
	style.TabRounding       = 2.0f;
	style.ScrollbarRounding = 2.0f;
	style.WindowBorderSize  = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.WindowPadding     = ImVec2(8, 8);
	style.FramePadding      = ImVec2(6, 4);
	style.ItemSpacing       = ImVec2(8, 5);
	style.IndentSpacing     = 18.0f;
	style.ScrollbarSize     = 13.0f;

	ImVec4* c = style.Colors;
	const ImVec4 panel  = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
	const ImVec4 panelH = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
	const ImVec4 panelA = ImVec4(0.32f, 0.32f, 0.32f, 1.0f);
	const ImVec4 accent = ImVec4(0.24f, 0.52f, 0.88f, 1.0f); // Unityの青
	const ImVec4 accentH= ImVec4(0.30f, 0.60f, 0.95f, 1.0f);

	c[ImGuiCol_Text]                 = ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
	c[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
	c[ImGuiCol_WindowBg]             = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
	c[ImGuiCol_ChildBg]              = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
	c[ImGuiCol_PopupBg]              = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	c[ImGuiCol_Border]               = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
	c[ImGuiCol_FrameBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
	c[ImGuiCol_FrameBgHovered]       = panelH;
	c[ImGuiCol_FrameBgActive]        = panelA;
	c[ImGuiCol_TitleBg]              = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	c[ImGuiCol_TitleBgActive]        = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
	c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	c[ImGuiCol_MenuBarBg]            = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
	c[ImGuiCol_ScrollbarBg]          = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.34f, 0.34f, 0.34f, 1.0f);
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
	c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.46f, 0.46f, 0.46f, 1.0f);
	c[ImGuiCol_CheckMark]            = accent;
	c[ImGuiCol_SliderGrab]           = accent;
	c[ImGuiCol_SliderGrabActive]     = accentH;
	c[ImGuiCol_Button]               = panel;
	c[ImGuiCol_ButtonHovered]        = panelH;
	c[ImGuiCol_ButtonActive]         = panelA;
	c[ImGuiCol_Header]               = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);
	c[ImGuiCol_HeaderHovered]        = ImVec4(0.32f, 0.32f, 0.32f, 1.0f);
	c[ImGuiCol_HeaderActive]         = accent;
	c[ImGuiCol_Separator]            = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
	c[ImGuiCol_SeparatorHovered]     = accent;
	c[ImGuiCol_SeparatorActive]      = accentH;
	c[ImGuiCol_ResizeGrip]           = ImVec4(0.30f, 0.30f, 0.30f, 0.6f);
	c[ImGuiCol_ResizeGripHovered]    = accent;
	c[ImGuiCol_ResizeGripActive]     = accentH;
	c[ImGuiCol_Tab]                  = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	c[ImGuiCol_TabHovered]           = accent;
	c[ImGuiCol_TabActive]            = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
	c[ImGuiCol_TextSelectedBg]       = ImVec4(accent.x, accent.y, accent.z, 0.45f);
#ifdef IMGUI_HAS_DOCK
	c[ImGuiCol_DockingPreview]       = ImVec4(accent.x, accent.y, accent.z, 0.55f);
	c[ImGuiCol_DockingEmptyBg]       = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
#endif
}
} // namespace
#endif // USE_IMGUI

namespace TuboEngine {
ImGuiManager* ImGuiManager::instance = nullptr; // シングルトンインスタンス
void ImGuiManager::Initialize() {

#ifdef USE_IMGUI
	// ImGuiのコンテキストを作成
	ImGui::CreateContext();
	// ImGuiのスタイルを設定
	ImGui::StyleColorsDark();
	ApplyUnityStyle(); // Unity(ダークスキン)風の配色・形状へ上書き

#ifdef IMGUI_HAS_DOCK
	// ウィンドウのドッキング(合体)を有効化。docking ブランチの ImGui が必要。
	// 有効にすると、ウィンドウのタイトルバーを他ウィンドウへドラッグして
	// タブ結合・上下左右への分割ドッキングができるようになる。
	// （非 docking 版では IMGUI_HAS_DOCK が未定義なので、このブロックは無効）
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

	// WinAppが正しく初期化されているか確認
	assert(TuboEngine::WinApp::GetInstance()->GetHWND() != nullptr);
	// ImGuiのDirectX12の初期化
	ImGui_ImplWin32_Init(TuboEngine::WinApp::GetInstance()->GetHWND());

	// descriptorHeapの設定
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// descriptorHeapの生成
	HRESULT result = TuboEngine::DirectXCommon::GetInstance()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap));
	assert(SUCCEEDED(result));

	// 日本語フォントの設定 (ImGui_ImplDX12_Init より前に行う必要がある)
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF(
	    "C:/Windows/Fonts/msgothic.ttc",
	    16.0f,
	    nullptr,
	    io.Fonts->GetGlyphRangesJapanese()
	);

	ImGui_ImplDX12_Init(
	    TuboEngine::DirectXCommon::GetInstance()->GetDevice().Get(), static_cast<int>(TuboEngine::DirectXCommon::GetInstance()->GetBackBufferCount()), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, srvHeap.Get(),
	    srvHeap->GetCPUDescriptorHandleForHeapStart(), srvHeap->GetGPUDescriptorHandleForHeapStart());
#endif // USE_IMGUI
}

void ImGuiManager::Finalize() {

#ifdef USE_IMGUI
	// 後始末
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	delete instance;
	instance = nullptr;

#endif // USE_IMGUI
}

void ImGuiManager::Begin() {

#ifdef USE_IMGUI
	// ImGuiの受付開始
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

#ifdef IMGUI_HAS_DOCK
	// 上部メニューバー。ドックスペースより先に出して作業領域を確保する。
	if (ImGui::BeginMainMenuBar()) {
		ImGui::TextDisabled("TuboEngine");
		ImGui::Separator();
		if (ImGui::BeginMenu("Window")) {
			// 全デバッグウィンドウの一括表示/非表示（これをOFFにすれば全部消える）
			ImGui::MenuItem("全ウィンドウ表示", nullptr, &debugWindowsVisible_);
			ImGui::Separator();
			ImGui::TextDisabled("個別パネル:");
			if (panelOpen_.empty()) {
				ImGui::TextDisabled("(まだ登録されていません)");
			}
			// 登録済みパネルを一覧表示。チェックで表示/非表示（収納）を切り替え。
			for (auto& [name, open] : panelOpen_) {
				ImGui::MenuItem(name.c_str(), nullptr, &open);
			}
			ImGui::EndMenu();
		}
		// 右側にFPSを表示
		ImGui::SameLine(ImGui::GetWindowWidth() - 96.0f);
		ImGui::Text("FPS %.0f", ImGui::GetIO().Framerate);
		ImGui::EndMainMenuBar();
	}

	// メインビューポート全体をドックスペース化する。
	// PassthruCentralNode により中央(空き領域)は透過 → 3D/ゲーム画面が見える(Unityのシーンビュー風)。
	// 各デバッグウィンドウはこの周囲にドッキングして配置できる。
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
#endif // IMGUI_HAS_DOCK
#endif // USE_IMGUI
}

void ImGuiManager::End() {
#ifdef USE_IMGUI
	// ImGuiの受付終了
	ImGui::Render();
#endif // USE_IMGUI
}

void ImGuiManager::Draw() {

#ifdef USE_IMGUI
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();

	// ディスクリプタヒープの配列をセットするコマンド
	ID3D12DescriptorHeap* ppHeaps[] = {srvHeap.Get()};
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// ImGuiの描画
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
#endif // USE_IMGUI
}

bool* ImGuiManager::PanelPtr(const char* name) {
	// 未登録なら「表示中(true)」で登録。Window メニューにこの名前が並ぶ。
	auto result = panelOpen_.try_emplace(name, true);
	return &result.first->second;
}
} // namespace TuboEngine