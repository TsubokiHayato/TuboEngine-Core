#include "ImGuiManager.h"
#include "SceneManager.h" // メニューバーの「Scene」メニューでシーン切り替えを呼ぶ
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_internal.h" // DockBuilder（初期レイアウト構築用）
#endif // USE_IMGUI
#include "Input.h"
#include "EditorPanels.h"
#include "EditorObjects.h"
#include "EditorTimeline.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

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
	desc.NumDescriptors = 2; // 0=フォント, 1=ゲームビュー用テクスチャ
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

	// Game ウィンドウの表示範囲は DrawGameWindow で今フレーム分を設定し直す
	gameViewRectValid_ = false;
	// シーンが ImGuiDraw で登録するオブジェクトを、毎フレーム登録し直す
	Editor::BeginFrame();

#ifdef IMGUI_HAS_DOCK
	// ショートカット（Unity 準拠）。テキスト入力中は無効。
	if (!ImGui::GetIO().WantTextInput) {
		// F1: Unity 風レイアウト ⇔ 従来(ゲーム画面の上にウィンドウを重ねる)
		if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
			gameViewEnabled_ = !gameViewEnabled_;
		}
		// Ctrl+Shift+P: 一時停止/再開、Ctrl+Alt+P: 1フレーム進める
		const ImGuiIO& io = ImGui::GetIO();
		if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_P, false)) {
			paused_ = !paused_;
		}
		if (io.KeyCtrl && io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_P, false)) {
			paused_ = true;
			stepRequested_ = true;
		}
	}

	// 上部メニューバー。ドックスペースより先に出して作業領域を確保する。
	if (ImGui::BeginMainMenuBar()) {
		ImGui::TextDisabled("TuboEngine");
		ImGui::Separator();
		if (ImGui::BeginMenu("Window")) {
			// 全デバッグウィンドウの一括表示/非表示（これをOFFにすれば全部消える）
			ImGui::MenuItem("全ウィンドウ表示", nullptr, &debugWindowsVisible_);
			ImGui::Separator();
			ImGui::TextDisabled("エディタ:");
			ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy_);
			ImGui::MenuItem("Inspector", nullptr, &showInspector_);
			ImGui::MenuItem("Project", nullptr, &showProject_);
			ImGui::MenuItem("Console", nullptr, &showConsole_);
			ImGui::MenuItem("Timeline", nullptr, &showTimeline_);
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
		// Layout メニュー：Unity 風レイアウト（ゲーム画面を Game ウィンドウ内に表示）の切り替え
		if (ImGui::BeginMenu("Layout")) {
			ImGui::MenuItem("Game ウィンドウに表示 (Unity風)", "F1", &gameViewEnabled_);
			if (ImGui::MenuItem("レイアウトをリセット")) {
				resetLayoutRequested_ = true;
			}
			ImGui::Separator();
			// マウスカーソルの固定方法（Esc で一時解除、Game 画面クリックで再固定）
			if (ImGui::BeginMenu("マウスカーソル")) {
				TuboEngine::WinApp* winApp = TuboEngine::WinApp::GetInstance();
				const TuboEngine::WinApp::CursorMode mode = winApp->GetCursorMode();
				if (ImGui::MenuItem("固定しない (Free)", nullptr, mode == TuboEngine::WinApp::CursorMode::Free)) {
					winApp->SetCursorMode(TuboEngine::WinApp::CursorMode::Free);
				}
				if (ImGui::MenuItem("ウィンドウ内に固定 (Confined)", nullptr, mode == TuboEngine::WinApp::CursorMode::Confined)) {
					winApp->SetCursorMode(TuboEngine::WinApp::CursorMode::Confined);
				}
				if (ImGui::MenuItem("中央に固定・非表示 (Locked)", nullptr, mode == TuboEngine::WinApp::CursorMode::Locked)) {
					winApp->SetCursorMode(TuboEngine::WinApp::CursorMode::Locked);
				}
				ImGui::Separator();
				ImGui::TextDisabled("Esc: 一時解除 / Game 画面クリック: 再固定");
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		// Scene メニュー：登録済みシーンの一覧から切り替え（SceneManager が中身を描く）
		if (ImGui::BeginMenu("Scene")) {
			SceneManager::GetInstance()->DrawSceneMenuItems();
			ImGui::EndMenu();
		}
		// 中央に 再生/一時停止/コマ送り/リロード のツールバー
		DrawToolbar();
		// 右側にFPSを表示
		ImGui::SameLine(ImGui::GetWindowWidth() - 96.0f);
		ImGui::Text("FPS %.0f", ImGui::GetIO().Framerate);
		ImGui::EndMainMenuBar();
	}

	// メインビューポート全体をドックスペース化する（ID固定＝imgui.ini にレイアウトが保存される）。
	const ImGuiID dockspaceId = ImGui::GetID("TuboEngineDockSpace");
	if (IsGameViewActive()) {
		// 起動直後に一度だけ、保存済みレイアウトが今の構成(Hierarchy等あり)か確認する
		if (!layoutChecked_) {
			layoutChecked_ = true;
			const ImGuiWindowSettings* hierarchySettings = ImGui::FindWindowSettingsByID(ImHashStr("Hierarchy"));
			if (!hierarchySettings || hierarchySettings->DockId == 0) {
				resetLayoutRequested_ = true;
			}
		}
		// 初回(保存済みレイアウト無し) or リセット要求時に初期レイアウトを組む
		ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspaceId);
		if (resetLayoutRequested_ || !root || !root->IsSplitNode()) {
			BuildDefaultLayout(dockspaceId);
			resetLayoutRequested_ = false;
		}
		// ゲーム画面は Game ウィンドウ内に描くので、中央も通常のドック領域にする
		ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport());
		DrawGameWindow();
		// Hierarchy / Inspector / Timeline は、シーンがオブジェクトを登録した後（End）で描く
		if (showProject_) {
			Editor::DrawProjectWindow(&showProject_);
		}
		if (showConsole_) {
			Editor::DrawConsoleWindow(&showConsole_);
		}
		AutoDockFloatingWindows();
	} else {
		// 従来モード：中央(空き領域)は透過 → 背面に全画面描画したゲーム画面が見える
		ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		// ゲーム画面がウィンドウ全体なのでマウス座標の補正は不要
		Input::GetInstance()->SetMouseRemap({0.0f, 0.0f}, {1.0f, 1.0f});
	}
#endif // IMGUI_HAS_DOCK
#endif // USE_IMGUI
}

void ImGuiManager::End() {
#ifdef USE_IMGUI
#ifdef IMGUI_HAS_DOCK
	if (IsGameViewActive()) {
		DrawLateEditorWindows();
	}
#endif // IMGUI_HAS_DOCK
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

void ImGuiManager::SetGameViewTexture(ID3D12Resource* texture) {
	gameViewTexture_ = texture;
#ifdef USE_IMGUI
	if (!texture) {
		return;
	}
	// ImGui 用ヒープのスロット1にゲームビュー用テクスチャの SRV を作る
	auto device = TuboEngine::DirectXCommon::GetInstance()->GetDevice();
	const UINT increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
	cpu.ptr += increment;
	gameViewSrvGpu_ = srvHeap->GetGPUDescriptorHandleForHeapStart();
	gameViewSrvGpu_.ptr += increment;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = texture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(texture, &srvDesc, cpu);
#endif // USE_IMGUI
}

bool ImGuiManager::IsGameViewActive() const {
#if defined(USE_IMGUI) && defined(IMGUI_HAS_DOCK)
	// 「全ウィンドウ表示」OFF の時は従来通りゲーム画面を全画面表示する
	return gameViewEnabled_ && debugWindowsVisible_ && gameViewTexture_ != nullptr;
#else
	return false;
#endif
}

bool ImGuiManager::GetGameViewRect(RECT& outRect) const {
	if (!IsGameViewActive() || !gameViewRectValid_) {
		return false;
	}
	outRect = gameViewRect_;
	return true;
}

bool ImGuiManager::ConsumeSceneUpdate() {
	// 再生中は毎フレーム更新。一時停止中はコマ送り要求があった時だけ1回更新する。
	if (!paused_) {
		return true;
	}
	if (stepRequested_) {
		stepRequested_ = false;
		return true;
	}
	return false;
}

#if defined(USE_IMGUI) && defined(IMGUI_HAS_DOCK)
namespace {
// エディタ自身のウィンドウ名（Hierarchy への列挙や自動ドッキングの対象外）
constexpr const char* kEditorWindowNames[] = {"Game", "Hierarchy", "Inspector", "Project", "Console", "Timeline", "Debug##Default"};

// デバッグウィンドウ（シーンや各マネージャが出すウィンドウ）として扱うか
bool IsDebugWindow(const ImGuiWindow* window) {
	// ドッキング中のウィンドウは ParentWindow がドックノードのホストになるので、それは除外しない
	if (window->ParentWindow != nullptr && !window->DockIsActive) {
		return false;
	}
	// （ドッキング中は ChildWindow フラグも付くので同様に除外しない）
	const ImGuiWindowFlags excluded =
	    (window->DockIsActive ? 0 : ImGuiWindowFlags_ChildWindow) | ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoDocking;
	if (window->Flags & excluded) {
		return false;
	}
	if (std::strncmp(window->Name, "##", 2) == 0) {
		return false;
	}
	for (const char* name : kEditorWindowNames) {
		if (std::strcmp(window->Name, name) == 0) {
			return false;
		}
	}
	return true;
}
} // namespace

void ImGuiManager::BuildDefaultLayout(ImGuiID dockspaceId) {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

	// Unity の既定レイアウトと同じ分割:
	//   右(全高)=Inspector / 下=Project・Console / 左上=Hierarchy / 中央=Game
	ImGuiID centerId = dockspaceId;
	const ImGuiID rightId = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, 0.24f, nullptr, &centerId);
	const ImGuiID bottomId = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, 0.30f, nullptr, &centerId);
	const ImGuiID leftId = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, 0.20f, nullptr, &centerId);

	ImGui::DockBuilderDockWindow("Game", centerId);
	ImGui::DockBuilderDockWindow("Hierarchy", leftId);
	ImGui::DockBuilderDockWindow("Inspector", rightId);
	ImGui::DockBuilderDockWindow("Project", bottomId);
	ImGui::DockBuilderDockWindow("Console", bottomId);
	ImGui::DockBuilderDockWindow("Timeline", bottomId);
	// 既に存在するデバッグウィンドウは Inspector の隣(タブ)へまとめる
	for (ImGuiWindow* window : GImGui->Windows) {
		if (IsDebugWindow(window)) {
			ImGui::DockBuilderDockWindow(window->Name, rightId);
		}
	}
	ImGui::DockBuilderFinish(dockspaceId);
	autoDockedWindows_.clear();
}

void ImGuiManager::DrawToolbar() {
	const float height = ImGui::GetFrameHeight();
	const float width = height * 1.6f;
	const float spacing = 4.0f;
	const float total = width * 4.0f + spacing * 3.0f;
	ImGui::SameLine((ImGui::GetWindowWidth() - total) * 0.5f);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 iconColor = IM_COL32(220, 220, 220, 255);

	// icon: 0=再生 1=一時停止 2=コマ送り 3=リロード
	auto button = [&](const char* id, int icon, bool active, const char* tooltip) {
		const ImVec2 p = ImGui::GetCursorScreenPos();
		const bool pressed = ImGui::InvisibleButton(id, ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		const ImVec4 bg = active ? ImVec4(0.24f, 0.52f, 0.88f, 1.0f) : hovered ? ImVec4(0.32f, 0.32f, 0.32f, 1.0f) : ImVec4(0.27f, 0.27f, 0.27f, 1.0f);
		drawList->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(bg), 3.0f);

		const ImVec2 c(p.x + width * 0.5f, p.y + height * 0.5f);
		const float r = height * 0.26f;
		switch (icon) {
		case 0:
			drawList->AddTriangleFilled(ImVec2(c.x - r * 0.8f, c.y - r), ImVec2(c.x - r * 0.8f, c.y + r), ImVec2(c.x + r, c.y), iconColor);
			break;
		case 1:
			drawList->AddRectFilled(ImVec2(c.x - r * 0.8f, c.y - r), ImVec2(c.x - r * 0.25f, c.y + r), iconColor);
			drawList->AddRectFilled(ImVec2(c.x + r * 0.25f, c.y - r), ImVec2(c.x + r * 0.8f, c.y + r), iconColor);
			break;
		case 2:
			drawList->AddTriangleFilled(ImVec2(c.x - r, c.y - r), ImVec2(c.x - r, c.y + r), ImVec2(c.x + r * 0.5f, c.y), iconColor);
			drawList->AddRectFilled(ImVec2(c.x + r * 0.55f, c.y - r), ImVec2(c.x + r, c.y + r), iconColor);
			break;
		default:
			drawList->PathArcTo(c, r, 0.6f, 5.6f, 16);
			drawList->PathStroke(iconColor, ImDrawFlags_None, 2.0f);
			{
				// 矢印の先端
				const ImVec2 tip(c.x + r * std::cos(0.6f), c.y + r * std::sin(0.6f));
				drawList->AddTriangleFilled(ImVec2(tip.x - r * 0.55f, tip.y), ImVec2(tip.x + r * 0.45f, tip.y - r * 0.15f), ImVec2(tip.x, tip.y + r * 0.6f), iconColor);
			}
			break;
		}
		if (hovered) {
			ImGui::SetTooltip("%s", tooltip);
		}
		ImGui::SameLine(0.0f, spacing);
		return pressed;
	};

	if (button("##play", 0, !paused_, "再生 / 再開 (Ctrl+Shift+P)")) {
		paused_ = false;
	}
	if (button("##pause", 1, paused_, "一時停止 (Ctrl+Shift+P)")) {
		paused_ = !paused_;
	}
	if (button("##step", 2, false, "1フレーム進める (Ctrl+Alt+P)")) {
		paused_ = true;
		stepRequested_ = true;
	}
	if (button("##reload", 3, false, "シーンをリロード")) {
		SceneManager::GetInstance()->ChangeScene(SceneManager::GetInstance()->GetCurrentSceneNo());
		// 一時停止中でもシーン切り替えを処理させる
		stepRequested_ = paused_;
	}
}

void ImGuiManager::DrawGameWindow() {
	TuboEngine::Input* input = TuboEngine::Input::GetInstance();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	const ImGuiWindowFlags flags =
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
	const bool visible = ImGui::Begin("Game", nullptr, flags);
	ImGui::PopStyleVar();

	if (!visible) {
		// タブが裏に隠れている等で表示されていない
		input->SetMouseRemap({0.0f, 0.0f}, {1.0f, 1.0f});
		ImGui::End();
		return;
	}

	// ゲーム解像度（＝テクスチャサイズ）の縦横比を保って領域いっぱいに表示（余白は黒帯）
	const D3D12_RESOURCE_DESC desc = gameViewTexture_->GetDesc();
	const float texW = static_cast<float>(desc.Width);
	const float texH = static_cast<float>(desc.Height);
	const ImVec2 areaMin = ImGui::GetCursorScreenPos();
	const ImVec2 area = ImGui::GetContentRegionAvail();
	const float scale = (std::max)(0.0f, (std::min)(area.x / texW, area.y / texH));
	gameViewDisplayScale_ = scale;
	const ImVec2 size(texW * scale, texH * scale);
	const ImVec2 imageMin(std::floor(areaMin.x + (area.x - size.x) * 0.5f), std::floor(areaMin.y + (area.y - size.y) * 0.5f));
	const ImVec2 imageMax(imageMin.x + size.x, imageMin.y + size.y);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(areaMin, ImVec2(areaMin.x + area.x, areaMin.y + area.y), IM_COL32(0, 0, 0, 255));
	drawList->AddImage(static_cast<ImTextureID>(gameViewSrvGpu_.ptr), imageMin, imageMax);
	// 一時停止中は分かるよう表示する
	if (paused_) {
		const char* label = "一時停止中";
		const ImVec2 textSize = ImGui::CalcTextSize(label);
		const ImVec2 p(imageMin.x + 8.0f, imageMin.y + 8.0f);
		drawList->AddRectFilled(p, ImVec2(p.x + textSize.x + 12.0f, p.y + textSize.y + 6.0f), IM_COL32(0, 0, 0, 170), 3.0f);
		drawList->AddText(ImVec2(p.x + 6.0f, p.y + 3.0f), IM_COL32(255, 200, 60, 255), label);
	}
	// 領域を確保（カーソルを進めるだけ）
	ImGui::Dummy(area);

	// Game ウィンドウ上のマウス操作はゲーム側に渡す（ImGui が入力を奪わないようにする）
	if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(imageMin, imageMax)) {
		ImGui::SetNextFrameWantCaptureMouse(false);
	}

	// ゲーム側の GetMousePosition がゲーム解像度の座標になるよう補正
	if (size.x > 0.0f && size.y > 0.0f) {
		const ImVec2 viewportPos = ImGui::GetMainViewport()->Pos;
		input->SetMouseRemap({imageMin.x - viewportPos.x, imageMin.y - viewportPos.y}, {texW / size.x, texH / size.y});
		// マウスカーソルを閉じ込める範囲として覚えておく（クライアント座標）
		gameViewRect_ = {
		    static_cast<LONG>(imageMin.x - viewportPos.x), static_cast<LONG>(imageMin.y - viewportPos.y),
		    static_cast<LONG>(imageMax.x - viewportPos.x), static_cast<LONG>(imageMax.y - viewportPos.y)};
		gameViewRectValid_ = true;
		gameImageMin_[0] = imageMin.x;
		gameImageMin_[1] = imageMin.y;
		gameImageMax_[0] = imageMax.x;
		gameImageMax_[1] = imageMax.y;
	}
	ImGui::End();
}

void ImGuiManager::DrawHierarchyWindow() {
	if (!ImGui::Begin("Hierarchy", &showHierarchy_)) {
		ImGui::End();
		return;
	}

	// 選択したが、まだ存在しなかった(非表示だった)ウィンドウを前面へ
	if (!focusRequest_.empty() && ImGui::FindWindowByName(focusRequest_.c_str())) {
		ImGui::SetWindowFocus(focusRequest_.c_str());
		focusRequest_.clear();
	}

	// 検索欄
	static ImGuiTextFilter filter;
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputTextWithHint("##search", "検索", filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf))) {
		filter.Build();
	}

	// 列挙: 前フレームに表示されていたデバッグウィンドウ（生成順）＋ 非表示中の登録パネル
	std::vector<std::string> names;
	for (ImGuiWindow* window : GImGui->Windows) {
		if (window->WasActive && IsDebugWindow(window)) {
			names.emplace_back(window->Name);
		}
	}
	for (const auto& [name, open] : panelOpen_) {
		if (std::find(names.begin(), names.end(), name) == names.end()) {
			names.push_back(name);
		}
	}

	const std::string sceneName = SceneManager::GetInstance()->GetCurrentSceneName();
	if (ImGui::TreeNodeEx("##scene", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", sceneName.c_str())) {
		// シーンが Editor::Inspect で登録したオブジェクト（クリックで Inspector に表示）
		for (const Editor::EditorObject& object : Editor::GetObjects()) {
			if (!filter.PassFilter(object.name.c_str())) {
				continue;
			}
			ImGui::PushID(object.name.c_str());
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (selectedObject_ == object.name) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			ImGui::TreeNodeEx("##object", flags, "%s %s", object.is3D ? "[3D]" : "[2D]", object.name.c_str());
			if (ImGui::IsItemClicked()) {
				selectedObject_ = object.name;
				selectedWindow_.clear();
				focusRequest_ = "Inspector";
			}
			ImGui::PopID();
		}
		if (Editor::GetObjects().empty()) {
			ImGui::TextDisabled("(オブジェクト未登録: シーンの ImGuiDraw で Editor::Inspect)");
		}
		ImGui::Separator();
		for (const std::string& name : names) {
			if (!filter.PassFilter(name.c_str())) {
				continue;
			}
			ImGui::PushID(name.c_str());
			// BeginPanel で登録されたパネルは表示/非表示を切り替えられる
			auto it = panelOpen_.find(name);
			bool* open = (it != panelOpen_.end()) ? &it->second : nullptr;
			if (open) {
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 1.0f));
				ImGui::Checkbox("##visible", open);
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("表示 / 非表示");
				}
			} else {
				ImGui::Dummy(ImVec2(ImGui::GetFontSize() + 2.0f, ImGui::GetFontSize()));
			}
			ImGui::SameLine();

			const bool hidden = open && !*open;
			if (hidden) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			}
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (selectedWindow_ == name) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			ImGui::TreeNodeEx("##item", flags, "%s", name.c_str());
			if (hidden) {
				ImGui::PopStyleColor();
			}
			// クリックで選択 → そのウィンドウを(非表示なら表示して)前面へ
			if (ImGui::IsItemClicked()) {
				selectedWindow_ = name;
				selectedObject_.clear();
				if (open) {
					*open = true;
				}
				focusRequest_ = name;
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	// シーン切り替え（Scene メニューと同じ内容）
	ImGui::Spacing();
	if (ImGui::CollapsingHeader("シーン一覧")) {
		SceneManager::GetInstance()->DrawSceneMenuItems();
	}
	ImGui::End();
}

void ImGuiManager::DrawInspectorWindow() {
	if (!ImGui::Begin("Inspector", &showInspector_)) {
		ImGui::End();
		return;
	}

	// Hierarchy でオブジェクトを選んでいれば、その位置・回転・拡縮・色などを編集する
	if (!selectedObject_.empty()) {
		if (Editor::EditorObject* object = Editor::FindObject(selectedObject_)) {
			Editor::DrawObjectInspector(*object);
			ImGui::Spacing();
		} else {
			ImGui::TextDisabled("'%s' は今のシーンにありません", selectedObject_.c_str());
		}
	}

	ImGui::SeparatorText("選択中");
	if (selectedWindow_.empty()) {
		ImGui::TextDisabled("Hierarchy で項目を選択してください");
	} else {
		ImGui::Text("%s", selectedWindow_.c_str());
		auto it = panelOpen_.find(selectedWindow_);
		if (it != panelOpen_.end()) {
			ImGui::Checkbox("表示", &it->second);
			ImGui::SameLine();
		}
		if (ImGui::SmallButton("前面に表示")) {
			if (it != panelOpen_.end()) {
				it->second = true;
			}
			focusRequest_ = selectedWindow_;
		}
	}
	ImGui::TextWrapped("各デバッグウィンドウはこのエリアにタブで並びます。");

	ImGui::SeparatorText("シーン");
	ImGui::Text("現在: %s (%d)", SceneManager::GetInstance()->GetCurrentSceneName().c_str(), SceneManager::GetInstance()->GetCurrentSceneNo());

	ImGui::SeparatorText("実行");
	ImGui::Text("状態: %s", paused_ ? "一時停止中" : "再生中");
	const ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("FPS: %.1f  (%.2f ms)", io.Framerate, io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f);

	const TuboEngine::WinApp* winApp = TuboEngine::WinApp::GetInstance();
	static const char* kCursorModeNames[] = {"Free", "Confined", "Locked"};
	ImGui::Text("マウス: %s%s", kCursorModeNames[static_cast<int>(winApp->GetCursorMode())], winApp->IsCursorCaptured() ? "" : " (一時解除中)");

	ImGui::SeparatorText("Game ビュー");
	if (gameViewTexture_) {
		const D3D12_RESOURCE_DESC desc = gameViewTexture_->GetDesc();
		ImGui::Text("解像度: %llu x %u", static_cast<unsigned long long>(desc.Width), desc.Height);
		ImGui::Text("表示倍率: %.0f%%", gameViewDisplayScale_ * 100.0f);
	}
	ImGui::End();
}

void ImGuiManager::DrawLateEditorWindows() {
	if (showHierarchy_) {
		DrawHierarchyWindow();
	}
	if (showInspector_) {
		DrawInspectorWindow();
	}

	Editor::EditorObject* selected = selectedObject_.empty() ? nullptr : Editor::FindObject(selectedObject_);
	if (showTimeline_) {
		// 前の版のレイアウト（imgui.ini）には Timeline が無いので、初回だけ Console と同じ場所に入れる
		if (ImGuiWindow* console = ImGui::FindWindowByName("Console"); console && console->DockId != 0) {
			ImGui::SetNextWindowDockID(console->DockId, ImGuiCond_FirstUseEver);
		}
		Editor::DrawTimelineWindow(&showTimeline_, selected);
	}

	// 選択中の 3D オブジェクトに移動用の矢印を出す
	if (selected && gameViewRectValid_) {
		if (TuboEngine::Camera* camera = SceneManager::GetInstance()->GetMainCamera()) {
			Editor::DrawGizmo(*selected, *camera, gameImageMin_[0], gameImageMin_[1], gameImageMax_[0], gameImageMax_[1]);
		}
	}
}

void ImGuiManager::AutoDockFloatingWindows() {
	// ドック先＝Inspector ウィンドウがいるドックノード（Inspector を閉じている/浮かせている時は何もしない）
	ImGuiWindow* inspector = ImGui::FindWindowByName("Inspector");
	if (!inspector || inspector->DockId == 0) {
		return;
	}
	const ImGuiID targetId = inspector->DockId;

	// 走査中に Windows を書き換えないよう、先に対象の名前を集める
	std::vector<const char*> names;
	for (ImGuiWindow* window : GImGui->Windows) {
		if (!window->WasActive || window->DockNode != nullptr || window->DockId != 0 || !IsDebugWindow(window)) {
			continue;
		}
		// 1セッションで1回だけ（ユーザーが自分で切り離したウィンドウは戻さない）
		if (std::find(autoDockedWindows_.begin(), autoDockedWindows_.end(), window->ID) != autoDockedWindows_.end()) {
			continue;
		}
		autoDockedWindows_.push_back(window->ID);
		names.push_back(window->Name);
	}
	for (const char* name : names) {
		ImGui::DockBuilderDockWindow(name, targetId);
	}
}
#endif // USE_IMGUI && IMGUI_HAS_DOCK

bool* ImGuiManager::PanelPtr(const char* name) {
	// 未登録なら「表示中(true)」で登録。Window メニューにこの名前が並ぶ。
	auto result = panelOpen_.try_emplace(name, true);
	return &result.first->second;
}

bool ImGuiManager::BeginPanel(const char* name) {
#ifdef USE_IMGUI
	bool* open = PanelPtr(name);
	// フラグが false なら ImGui::Begin を呼ばない＝ウィンドウ非表示。
	// （ImGui::Begin は *p_open==false でも自動で隠さないため、呼び出し側でスキップする必要がある）
	if (!*open) {
		panelBeganStack_.push_back(false);
		return false;
	}
	const bool visible = ImGui::Begin(name, open); // ✕ボタンも出る（押すと *open=false）
	panelBeganStack_.push_back(true);
	return visible;
#else
	(void)name;
	return false;
#endif
}

void ImGuiManager::EndPanel() {
#ifdef USE_IMGUI
	if (panelBeganStack_.empty()) return;
	const bool began = panelBeganStack_.back();
	panelBeganStack_.pop_back();
	// ImGui::Begin を呼んだ時だけ End する（呼んでいなければ End してはいけない）
	if (began) ImGui::End();
#endif
}
} // namespace TuboEngine