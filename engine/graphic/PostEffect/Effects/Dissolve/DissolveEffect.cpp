#include "DissolveEffect.h"
#include"ImGuiManager.h"

DissolveEffect::DissolveEffect() = default;
DissolveEffect::~DissolveEffect() {
	if (cbResource_ && params_) {
		cbResource_->Unmap(0, nullptr);
		params_ = nullptr;
	}
}
void DissolveEffect::Initialize() {
	// PSO初期化
	pso_ = std::make_unique<DissolvePSO>();
	pso_->Initialize();
	// 定数バッファ作成
	cbResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(DissolveParams));
	cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&params_));
	// デフォルト値
	params_->dissolveThreshold = 0.5f;
	params_->edgeColor = { 1.0f, 0.0f, 0.0f }; // 赤色
	params_->edgeStrength = 1.0f; // エッジの強さ
	params_->edgeWidth = 0.1f; // エッジの幅

	

	// マスクテクスチャのリソース生成
	DirectX::ScratchImage mipImages = TuboEngine::DirectXCommon::LoadTexture(maskTextureFileName_);
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	maskTextureResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateTextureResource(metadata);
	
	maskTextureUploadResource_ = TuboEngine::DirectXCommon::GetInstance()->UploadTextureData(maskTextureResource_, mipImages);



	// SRV作成（インデックス1にSRVを作成）
	D3D12_SHADER_RESOURCE_VIEW_DESC maskTextureSRVDesc{};
	maskTextureSRVDesc.Format = maskTextureResource_->GetDesc().Format;
	maskTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	maskTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	maskTextureSRVDesc.Texture2D.MipLevels = 1;

	TuboEngine::DirectXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
		maskTextureResource_.Get(), &maskTextureSRVDesc, TuboEngine::DirectXCommon::GetInstance()->GetSRVCPUDescriptorHandle(1)
	);

}
void DissolveEffect::Update() {
	

}
void DissolveEffect::DrawImGui() {

#ifdef USE_IMGUI
	ImGui::Begin("Dissolve Effect");
	ImGui::SliderFloat("Dissolve Threshold", &params_->dissolveThreshold, 0.0f, 1.0f);
	ImGui::ColorEdit3("Edge Color", &params_->edgeColor.x);
	ImGui::SliderFloat("Edge Strength", &params_->edgeStrength, 0.0f, 5.0f);
	ImGui::SliderFloat("Edge Width", &params_->edgeWidth, 0.0f, 1.0f);
	ImGui::End();
#endif // USE_IMGUI
}
void DissolveEffect::Draw(ID3D12GraphicsCommandList* commandList) {
	// PSO・ルートシグネチャ設定
	pso_->DrawSettingsCommon();
	// SRVやCBVのバインドはマネージャ側で行う場合は不要
	// ここでCBVをバインドする場合:
	commandList->SetGraphicsRootConstantBufferView(1, cbResource_->GetGPUVirtualAddress());
}

