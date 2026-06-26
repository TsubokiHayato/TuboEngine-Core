#include "DepthBasedOutlineEffect.h"
#include "ImGuiManager.h"

void DepthBasedOutlineEffect::Initialize() {
	// PSO初期化
	pso_ = std::make_unique<DepthBasedOutlinePSO>();
	pso_->Initialize();

	// 定数バッファ作成（projectionMatrix用）
	materialCB_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(ToonDepthOutlineParams));
	materialCB_->Map(0, nullptr, reinterpret_cast<void**>(&materialCBData_));
	// 初期値（単位行列）
	materialCBData_->projectionInverse = TuboEngine::Math::MakeIdentity4x4();
	materialCBData_->outlineColor = {0.0f, 0.0f, 0.0f, 1.0f}; // アウトラインの色（黒)
	materialCBData_->outlineThickness = 1.0f;                 // アウトラインの太さ
	materialCBData_->outlineDepthThreshold = 1.0f;           // アウトラインの深度しきい値


	
	// SRV作成（インデックス4に深度SRVを作成）
	// ※ SRVヒープの割り当て: slot0=シーン, slot1=Dissolveマスク, slot2/3=ping-pong中間,
	//    なので深度は slot4 を使う（slot2 だと ping-pong テクスチャに上書きされて読めない）。
	D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSRVDesc{};
	depthTextureSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthTextureSRVDesc.Texture2D.MipLevels = 1;

	TuboEngine::DirectXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
	    TuboEngine::DirectXCommon::GetInstance()->GetDepthStencliResouece().Get(), &depthTextureSRVDesc, TuboEngine::DirectXCommon::GetInstance()->GetSRVCPUDescriptorHandle(4));
}

void DepthBasedOutlineEffect::Update() {
	// カメラ未設定時に落ちないようガードする（SetMainCamera前にUpdateが来る場合がある）
	if (!camera_) {
		return;
	}
	// カメラのProjection逆行列を更新（深度→ビュー空間復元用）
	materialCBData_->projectionInverse = TuboEngine::Math::Inverse(camera_->GetProjectionMatrix());
}

void DepthBasedOutlineEffect::DrawImGui() {

#ifdef USE_IMGUI
	if (TuboEngine::ImGuiManager::GetInstance()->BeginPanel("DepthBasedOutline")) {
	// 必要に応じてImGuiでパラメータ調整
	ImGui::ColorEdit4("Outline Color", &materialCBData_->outlineColor.x);
	ImGui::SliderFloat("Outline Thickness", &materialCBData_->outlineThickness, 0.0f, 10.0f);
	ImGui::SliderFloat("Outline Depth Threshold", &materialCBData_->outlineDepthThreshold, 0.0f, 1.0f);

	}
	TuboEngine::ImGuiManager::GetInstance()->EndPanel();
#endif // USE_IMGUI
}

void DepthBasedOutlineEffect::SetMainCamera(TuboEngine::Camera* camera) {
	// カメラの設定を行う
	camera_ = camera;
	if (camera) {
		// ここでカメラのプロジェクション行列を取得し、materialCBData_にセットする
		materialCBData_->projectionInverse = TuboEngine::Math::Inverse(camera->GetProjectionMatrix());
	}
}

void DepthBasedOutlineEffect::Draw(ID3D12GraphicsCommandList* commandList) {
	pso_->DrawSettingsCommon();
	commandList->SetGraphicsRootConstantBufferView(1, materialCB_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, TuboEngine::DirectXCommon::GetInstance()->GetSRVGPUDescriptorHandle(4));
}