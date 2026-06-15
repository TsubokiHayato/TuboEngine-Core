#include "ToonEffect.h"
#include "ImGuiManager.h"
#include "Matrix.h"

ToonEffect::ToonEffect() = default;
ToonEffect::~ToonEffect() {
	if (toonCB_ && toonParams_) {
		toonCB_->Unmap(0, nullptr);
		toonParams_ = nullptr;
	}
}
void ToonEffect::Initialize() {
	pso_ = std::make_unique<ToonPSO>();
	pso_->Initialize();

	// 定数バッファ作成
	toonCB_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource((sizeof(ToonParams) + 255) & ~255);
	toonCB_->Map(0, nullptr, reinterpret_cast<void**>(&toonParams_));
	// デフォルト値
	toonParams_->stepCount = 3;
	toonParams_->toonRate = 0.5f; // トゥーンレートの初期値
	toonParams_->shadowColor = TuboEngine::Math::Vector3(0.0f, 0.0f, 0.0f); // シャドウカラーの初期値
	toonParams_->highlightColor = TuboEngine::Math::Vector3(0.0f, 0.0f, 0.0f); // シャドウカラーの初期値

	// SRV作成（インデックス1にSRVを作成）
	D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSRVDesc{};
	depthTextureSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthTextureSRVDesc.Texture2D.MipLevels = 1;

	TuboEngine::DirectXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
	    TuboEngine::DirectXCommon::GetInstance()->GetDepthStencliResouece().Get(), &depthTextureSRVDesc, TuboEngine::DirectXCommon::GetInstance()->GetSRVCPUDescriptorHandle(1));
}

void ToonEffect::Update() {
	
}

void ToonEffect::DrawImGui() {

#ifdef USE_IMGUI
	ImGui::Begin("ToonEffect");
	ImGui::SliderInt("Step Count", &toonParams_->stepCount,1, 20);
	ImGui::DragFloat("Toon Rate", &toonParams_->toonRate, 0.01f, 0.0f, 1.0f, "%.2f");
	ImGui::ColorEdit3("Shadow Color", &toonParams_->shadowColor.x);
	ImGui::ColorEdit3("Highlight Color", &toonParams_->highlightColor.x);
	ImGui::End();
#endif // USE_IMGUI
}
void ToonEffect::Draw(ID3D12GraphicsCommandList* commandList) {
	pso_->DrawSettingsCommon();
	// SRV等のセットはマネージャ側で
	commandList->SetGraphicsRootConstantBufferView(1, toonCB_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, TuboEngine::DirectXCommon::GetInstance()->GetSRVGPUDescriptorHandle(1));
}

void ToonEffect::SetMainCamera(TuboEngine::Camera* camera) {

// カメラの設定を行う
	camera_ = camera;
}
