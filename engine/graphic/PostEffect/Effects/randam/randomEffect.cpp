#include "randomEffect.h"
#include "ImGuiManager.h"
#include <random>
#include <chrono>

randomEffect::randomEffect() = default;
randomEffect::~randomEffect() {
    if (cbResource_ && params_) {
        cbResource_->Unmap(0, nullptr);
        params_ = nullptr;
    }
}

void randomEffect::Initialize() {
    // PSO初期化
    pso_ = std::make_unique<randomPSO>();
    pso_->Initialize();
    // 定数バッファ作成
	cbResource_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(RandomParams));
    cbResource_->Map(0, nullptr, reinterpret_cast<void**>(&params_));

    // 乱数エンジンで値を生成（シードは時間ベースで初期化）
    std::random_device rd;
    std::mt19937 randomEngine(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
   
    params_->time = 0.0f; // timeはUpdateで進める
}


void randomEffect::Update() {
    // 例: std::chrono を使った経過時間の取得
    static auto startTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float time = std::chrono::duration<float>(now - startTime).count();
    params_->time = time;
}


void randomEffect::DrawImGui() {

#ifdef USE_IMGUI
    ImGui::Begin("Random Effect");
    // timeは自動で進むので表示のみ
    ImGui::Text("Time: %.2f", params_->time);
   
    ImGui::End();
#endif // USE_IMGUI
}

void randomEffect::Draw(ID3D12GraphicsCommandList* commandList) {
    // PSO・ルートシグネチャ設定
    pso_->DrawSettingsCommon();
    // CBVをバインド
    commandList->SetGraphicsRootConstantBufferView(1, cbResource_->GetGPUVirtualAddress());
}
