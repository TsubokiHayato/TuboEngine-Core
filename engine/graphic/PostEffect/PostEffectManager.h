#include <vector>
#include <memory>
#include"PostEffectBase.h"
#include"Camera.h"

class PostEffectManager
{
public:
    void AddEffect(std::unique_ptr<PostEffectBase> effect);

    void InitializeAll();

    void UpdateAll();

    // エフェクト切り替え
    void SetCurrentEffect(size_t index);

    void DrawCurrent(ID3D12GraphicsCommandList* commandList);

    // 重ねがけ（チェーン）対応 ------------------------------------------------
    // 同時に適用できるエフェクトの上限（ping-pongは中間2枚で何段でも回せるが、
    // 暴走防止のため十分大きい上限を設ける）
    static constexpr size_t kMaxStack = 16;

    // 適用順リストを設定する（先頭から順に適用。effects_範囲外/上限超過は無視）
    void SetEnabledOrder(const std::vector<size_t>& order) {
        enabledOrder_.clear();
        for (size_t idx : order) {
            if (idx < effects_.size()) {
                enabledOrder_.push_back(idx);
            }
            if (enabledOrder_.size() >= kMaxStack) {
                break;
            }
        }
    }
    const std::vector<size_t>& GetEnabledOrder() const { return enabledOrder_; }

    // 指定indexのエフェクトのDraw（PSO/CBV/個別SRV）だけを呼ぶ
    void DrawAt(size_t index, ID3D12GraphicsCommandList* commandList) {
        if (index < effects_.size()) {
            effects_[index]->Draw(commandList);
        }
    }
    // ------------------------------------------------------------------------

    void DrawImGui();

    void SetMainCamera(TuboEngine::Camera* camera);

    size_t GetEffectCount() const { return effects_.size(); }
    size_t GetCurrentIndex() const { return currentIndex_; }

	template<typename T> T* GetEffect() {
		for (auto& effect : effects_) {
			if (auto ptr = dynamic_cast<T*>(effect.get())) {
				return ptr;
			}
		}
		return nullptr;
	}

private:
    std::vector<std::unique_ptr<PostEffectBase>> effects_;
    size_t currentIndex_ = 0;
    std::vector<size_t> enabledOrder_; // 重ねがけの適用順（最大kMaxStack枚）
};

