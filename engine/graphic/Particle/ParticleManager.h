#pragma once
#include "IParticleEmitter.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
class Camera;

namespace TuboEngine {
class ParticleManager {
public:
	static ParticleManager* GetInstance() {
		static ParticleManager inst;
		return &inst;
	}

	~ParticleManager() { Finalize(); }

	void Update(float dt, TuboEngine::Camera* defaultCam);
	void Draw();
	void DrawImGui();

	// Registry-based creation
	IParticleEmitter* CreateEmitterByType(const std::string& typeName, const ParticlePreset& preset);

	template<typename EmitterT> EmitterT* CreateEmitter(const ParticlePreset& preset) {
		ParticlePreset adjusted = preset;
		adjusted.name = GenerateUniqueName(adjusted.name.empty() ? "Emitter" : adjusted.name);
		auto ptr = std::make_unique<EmitterT>();
		ptr->Initialize(adjusted);
		EmitterT* raw = ptr.get();
		emitters_.push_back(std::move(ptr));
		SetStatus("Created '%s'", adjusted.name.c_str());
		MarkChanged();
		return raw;
	}

	IParticleEmitter* Find(const std::string& name);
	void Remove(const std::string& name);

	void SaveAll(const std::string& filePath);
	void LoadAll(const std::string& filePath);
	void SaveSelected(const std::string& filePath, const std::vector<std::string>& names);
	void LoadMerge(const std::string& filePath);

	void Undo();
	void Redo();
	void InitialLoad(const std::string& filePath);
	void Finalize() {
		emitters_.clear();
		previewEmitter_.reset();
		history_.clear();
		historyIndex_ = -1;
	}

private:
	ParticleManager();
	std::string GenerateUniqueName(const std::string& base) const;
	void CaptureHistory();
	void ApplySnapshot(const std::string& jsonStr);
	std::string BuildSnapshotJson() const;
	void SetStatus(const char* fmt, ...);
	void MarkChanged();
	void DrawStatusBar();
	void DrawTemplatesSection();
	void DrawEmittersSection();
	enum class PendingActionType { None, DeleteEmitter, ClearEmitter, LoadAll, LoadMergeSelected, UndoAction, RedoAction };
	PendingActionType pendingAction_ = PendingActionType::None;
	std::string pendingEmitterName_;
	std::string confirmMessage_;
	void OpenConfirmPopup(const char* popupName, const char* message);
	void ExecutePendingAction();

	// Preview helpers
	void UpdatePreview(float dt, Camera* cam);
	void DrawPreview(ID3D12GraphicsCommandList* cmd);
	void ApplyPreviewPreset(const ParticlePreset& src, int type);

	// Registry
	using EmitterFactoryFunc = std::function<std::unique_ptr<IParticleEmitter>()>;
	std::unordered_map<std::string, EmitterFactoryFunc> emitterRegistry_;
	void RegisterDefaultEmitters();
	std::string DetectEmitterType(IParticleEmitter* e) const;

private:
	std::vector<std::unique_ptr<IParticleEmitter>> emitters_;

	char statusMsg_[256]{};
	float statusTimer_ = 0.0f;

	std::vector<std::string> history_;
	int historyIndex_ = -1;
	bool changedThisFrame_ = false;

	std::string selectedEmitter_;
	bool initialLoaded_ = false;

	// プレビュー用
	std::unique_ptr<IParticleEmitter> previewEmitter_;
	int previewType_ = -1;         // 現在のプレビュー型
	ParticlePreset previewCached_; // 前回適用した値
	bool previewEnabled_ = false;
	bool previewNeedsRecreate_ = false;
	bool previewPendingDestroy_ = false; // LivePreview OFF時の遅延破棄
};

} // namespace TuboEngine