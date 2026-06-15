#include "ImGuiManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif // USE_IMGUI

#include <cassert>
namespace TuboEngine {
ImGuiManager* ImGuiManager::instance = nullptr; // シングルトンインスタンス
void ImGuiManager::Initialize() {

#ifdef USE_IMGUI
	// ImGuiのコンテキストを作成
	ImGui::CreateContext();
	// ImGuiのスタイルを設定
	ImGui::StyleColorsDark();
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
} // namespace TuboEngine