#pragma once
#include "IParticleEmitter.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
class Camera;

namespace TuboEngine {
/// <summary>
/// 全パーティクルエミッターの登録・更新・描画を統括するクラス。
/// </summary>
class ParticleManager {
public:
	/// <summary>
	/// シングルトンインスタンスの取得。
	/// </summary>
	static ParticleManager* GetInstance() {
		static ParticleManager inst;
		return &inst;
	}

	/// <summary>
	/// デストラクタ。
	/// </summary>
	~ParticleManager() { Finalize(); }

	/// <summary>
	/// 更新処理。
	/// </summary>
	void Update(float dt, TuboEngine::Camera* defaultCam);
	/// <summary>
	/// 描画処理。
	/// </summary>
	void Draw();
	/// <summary>
	/// ImGuiによるデバッグ表示。
	/// </summary>
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

	/// <summary>
	/// 名前からエミッターを検索する。
	/// </summary>
	IParticleEmitter* Find(const std::string& name);
	/// <summary>
	/// 指定エミッターを削除する。
	/// </summary>
	void Remove(const std::string& name);

	/// <summary>
	/// 全プリセットを保存する。
	/// </summary>
	void SaveAll(const std::string& filePath);
	/// <summary>
	/// All の読み込み。
	/// </summary>
	void LoadAll(const std::string& filePath);
	/// <summary>
	/// 選択中のプリセットを保存する。
	/// </summary>
	void SaveSelected(const std::string& filePath, const std::vector<std::string>& names);
	/// <summary>
	/// Merge の読み込み。
	/// </summary>
	void LoadMerge(const std::string& filePath);

	/// <summary>
	/// 直前の変更を取り消す。
	/// </summary>
	void Undo();
	/// <summary>
	/// 取り消した変更をやり直す。
	/// </summary>
	void Redo();
	/// <summary>
	/// 起動時のプリセット読み込み。
	/// </summary>
	void InitialLoad(const std::string& filePath);
	/// <summary>
	/// 終了処理。
	/// </summary>
	void Finalize() {
		emitters_.clear();
		previewEmitter_.reset();
		history_.clear();
		historyIndex_ = -1;
	}

private:
	/// <summary>
	/// コンストラクタ。
	/// </summary>
	ParticleManager();
	/// <summary>
	/// 重複しない名前を生成する。
	/// </summary>
	std::string GenerateUniqueName(const std::string& base) const;
	/// <summary>
	/// Undo用に現在の状態を履歴へ記録する。
	/// </summary>
	void CaptureHistory();
	/// <summary>
	/// Snapshot を適用する。
	/// </summary>
	void ApplySnapshot(const std::string& jsonStr);
	/// <summary>
	/// SnapshotJson を構築する。
	/// </summary>
	std::string BuildSnapshotJson() const;
	/// <summary>
	/// Status を設定する。
	/// </summary>
	void SetStatus(const char* fmt, ...);
	/// <summary>
	/// 変更ありフラグを立てる。
	/// </summary>
	void MarkChanged();
	/// <summary>
	/// StatusBar の描画。
	/// </summary>
	void DrawStatusBar();
	/// <summary>
	/// TemplatesSection の描画。
	/// </summary>
	void DrawTemplatesSection();
	/// <summary>
	/// EmittersSection の描画。
	/// </summary>
	void DrawEmittersSection();
	enum class PendingActionType { None, DeleteEmitter, ClearEmitter, LoadAll, LoadMergeSelected, UndoAction, RedoAction };
	PendingActionType pendingAction_ = PendingActionType::None;
	std::string pendingEmitterName_;
	std::string confirmMessage_;
	/// <summary>
	/// 確認ポップアップを開く。
	/// </summary>
	void OpenConfirmPopup(const char* popupName, const char* message);
	/// <summary>
	/// 保留中の操作を実行する。
	/// </summary>
	void ExecutePendingAction();

	// Preview helpers
	void UpdatePreview(float dt, Camera* cam);
	void DrawPreview(ID3D12GraphicsCommandList* cmd);
	void ApplyPreviewPreset(const ParticlePreset& src, int type);

	// Registry
	using EmitterFactoryFunc = std::function<std::unique_ptr<IParticleEmitter>()>;
	std::unordered_map<std::string, EmitterFactoryFunc> emitterRegistry_;
	/// <summary>
	/// デフォルトのエミッター群を登録する。
	/// </summary>
	void RegisterDefaultEmitters();
	/// <summary>
	/// プリセットからエミッター種別を判定する。
	/// </summary>
	std::string DetectEmitterType(IParticleEmitter* e) const;

private:
	std::vector<std::unique_ptr<IParticleEmitter>> emitters_;

	char statusMsg_[256]{};
	float statusTimer_ = 0.0f;

	std::vector<std::string> history_;
	int historyIndex_ = -1;
	bool changedThisFrame_ = false;
	bool applyingSnapshot_ = false; // ApplySnapshot 実行中フラグ（復元を履歴に記録しないため）
	bool hasPendingApply_ = false;  // Undo/Redo の復元を Draw 冒頭まで遅延するフラグ
	std::string pendingApplyJson_;  // 遅延適用する snapshot JSON

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