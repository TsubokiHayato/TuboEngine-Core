#include "SSAOEffect.h"
#include "ImGuiManager.h"

void SSAOEffect::Initialize() {
	// PSO初期化
	pso_ = std::make_unique<SSAOPSO>();
	pso_->Initialize();

	// 定数バッファ作成
	materialCB_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(SSAOParams));
	materialCB_->Map(0, nullptr, reinterpret_cast<void**>(&materialCBData_));
	// 初期値
	materialCBData_->projectionInverse = TuboEngine::Math::MakeIdentity4x4();
	materialCBData_->radius = 8.0f;      // サンプル半径（px）
	materialCBData_->bias = 0.02f;       // 自己遮蔽防止
	materialCBData_->intensity = 1.0f;   // AO の強さ
	materialCBData_->power = 1.5f;       // コントラスト
	materialCBData_->depthRange = 1.0f;  // 深度差の許容範囲（ビュー空間距離）

	// 深度SRVを作成（slot5）。DepthBasedOutline(slot4) と別枠にして衝突を避ける。
	D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSRVDesc{};
	depthTextureSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthTextureSRVDesc.Texture2D.MipLevels = 1;

	TuboEngine::DirectXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
	    TuboEngine::DirectXCommon::GetInstance()->GetDepthStencliResouece().Get(),
	    &depthTextureSRVDesc,
	    TuboEngine::DirectXCommon::GetInstance()->GetSRVCPUDescriptorHandle(kDepthSrvIndex));
}

void SSAOEffect::Update() {
	if (!camera_) {
		return;
	}
	// 深度→ビュー空間復元に使う射影逆行列を更新
	materialCBData_->projectionInverse = TuboEngine::Math::Inverse(camera_->GetProjectionMatrix());
}

void SSAOEffect::DrawImGui() {
#ifdef USE_IMGUI
	if (TuboEngine::ImGuiManager::GetInstance()->BeginPanel("SSAO")) {
		ImGui::SliderFloat("Radius (px)", &materialCBData_->radius, 1.0f, 32.0f);
		ImGui::SliderFloat("Bias", &materialCBData_->bias, 0.0f, 0.5f);
		ImGui::SliderFloat("Intensity", &materialCBData_->intensity, 0.0f, 3.0f);
		ImGui::SliderFloat("Power", &materialCBData_->power, 0.5f, 4.0f);
		ImGui::SliderFloat("Depth Range", &materialCBData_->depthRange, 0.05f, 5.0f);
	}
	TuboEngine::ImGuiManager::GetInstance()->EndPanel();
#endif // USE_IMGUI
}

void SSAOEffect::SetMainCamera(TuboEngine::Camera* camera) {
	camera_ = camera;
	if (camera) {
		materialCBData_->projectionInverse = TuboEngine::Math::Inverse(camera->GetProjectionMatrix());
	}
}

void SSAOEffect::Draw(ID3D12GraphicsCommandList* commandList) {
	pso_->DrawSettingsCommon();
	// b0: パラメータ, t1: 深度SRV(slot5)。t0(シーン) は OffScreenRendering 側で param0 にバインドされる。
	commandList->SetGraphicsRootConstantBufferView(1, materialCB_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, TuboEngine::DirectXCommon::GetInstance()->GetSRVGPUDescriptorHandle(kDepthSrvIndex));
}
