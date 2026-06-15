#include "PostEffectManager.h"
#include "Effects/DepthBasedOutline/DepthBasedOutlineEffect.h"
#include "Effects/Dissolve/DissolveEffect.h"

void PostEffectManager::AddEffect(std::unique_ptr<PostEffectBase> effect) {
    effects_.emplace_back(std::move(effect));
}

void PostEffectManager::InitializeAll() {
    for (auto& e : effects_) e->Initialize();
}

void PostEffectManager::UpdateAll() {
    for (auto& e : effects_) e->Update();
}

void PostEffectManager::SetCurrentEffect(size_t index) {
    if (index < effects_.size() && index != currentIndex_) {
        // DepthBasedOutlineEffectからDissolveEffectへの切り替えを禁止
        auto* current = effects_[currentIndex_].get();
        auto* next = effects_[index].get();
        if (dynamic_cast<DepthBasedOutlineEffect*>(current) && dynamic_cast<DissolveEffect*>(next)) {
            // 切り替え禁止
            return;
        }
        currentIndex_ = index;
    }
}

void PostEffectManager::DrawCurrent(ID3D12GraphicsCommandList* commandList) {
    if (currentIndex_ < effects_.size()) {
        effects_[currentIndex_]->Draw(commandList);
    }
}

void PostEffectManager::DrawImGui() {
    if (currentIndex_ < effects_.size()) {
        effects_[currentIndex_]->DrawImGui();
    }
}

void PostEffectManager::SetMainCamera(TuboEngine::Camera* camera) {
    for (auto& e : effects_) {
        e->SetMainCamera(camera);
    }
}

