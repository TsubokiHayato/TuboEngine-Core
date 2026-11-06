#pragma once
#include "Camera.h"
#include "Easing.h"
#include "KeyframeClip.h"
#include "Vector3.h"
#include <array>
#include <functional>
#include <string>
#include <vector>

// =============================================================================
//  UniversalCamera ── ターゲットを追従するカメラ（視点切り替え・ロックオン・シェイク・FOV・キーフレーム）
//
//    // Initialize
//    camera_.AddTarget("player", [this] { return player_->GetPosition(); }, [this] { return player_->GetRotation().y; });
//    camera_.AddTarget("boss", [this] { return boss_->GetPosition(); });
//    camera_.SetTarget("player", 0.0f);                       // 最初は切り替えアニメ無し
//    camera_.SetViewMode(UniversalCamera::ViewMode::Behind, 0.0f);
//    player_->SetCamera(camera_.GetCamera());                 // オブジェクトにはこれを渡す
//
//    // Update（ターゲットを動かした後、オブジェクトの Update の前）
//    camera_.Update(TuboEngine::Time::DeltaTime());
//
//    // GetMainCamera
//    return camera_.GetCamera();
//
//  ・SetTarget / SetViewMode はイージングでなめらかに切り替わる（途中で切り替えても跳ばない）
//  ・LockOn("boss") で自分とボスを同時に画面に収める。ClearLockOn() で解除
//  ・Shake(0.6f) で揺らす（強さ 0〜1。足していくと強くなり、時間で収まる）
//  ・KickFov(8.0f) で一瞬だけ画角を広げる（ダッシュ・被弾）。SetFovOffset で速度などに応じた上乗せ
//  ・PlayClip(&clip) でキーフレーム（Timeline で作った camera:eye / camera:lookAt / camera:fov）を再生
// =============================================================================

namespace TuboEngine {

class UniversalCamera {
public:
	/// <summary>
	/// 視点の種類（それぞれ Preset を持つ）。
	/// </summary>
	enum class ViewMode {
		Behind,         // 真後ろ（アーマード・コア風。肩越し・強めの追従）
		BehindDiagonal, // 頭上後ろ斜め（頭上の後ろから 45° で見下ろす。真上 = 90°）
		TopDown,        // 見下ろし（ターゲットの向きに関係なくワールドに固定）
		Count,
	};

	/// <summary>
	/// 視点ごとの設定（ImGui で調整して JSON に保存できる）。
	/// </summary>
	struct Preset {
		float distance = 6.0f;     // ターゲットからの距離
		float pitchDeg = 15.0f;    // 見下ろす角度（度）。0 = 水平、90 = 真上から
		float yawOffsetDeg = 0.0f; // ターゲットの真後ろからずらす角度（度）
		float lookHeight = 1.5f;   // 注視点の高さ（ターゲットの位置から上へ）
		float shoulder = 0.0f;     // 注視点の横ずれ（肩越し）
		float lookAhead = 0.0f;    // 注視点を前へ先読みする量
		bool followYaw = true;     // ターゲットの向きに合わせて回るか
		float followSpeed = 8.0f;  // 追従の速さ（大きいほどピッタリ付く）
		float fovDeg = 26.0f;      // 画角（度）
	};

	/// <summary>
	/// カメラの見え方1つ分（位置・注視点・画角）。切り替え・キーフレームの補間に使う。
	/// </summary>
	struct View {
		Math::Vector3 eye{0.0f, 3.0f, -8.0f};
		Math::Vector3 lookAt{0.0f, 1.0f, 0.0f};
		float fovDeg = 26.0f;
	};

	UniversalCamera();

	// --- 追従対象 ---

	/// <summary>
	/// 追従対象を名前で登録する（同名は上書き）。getYaw はターゲットの向き（Y 回転・ラジアン）。無ければ向きは使わない。
	/// </summary>
	void AddTarget(const std::string& name, std::function<Math::Vector3()> getPosition, std::function<float()> getYaw = nullptr);
	void RemoveTarget(const std::string& name);
	std::vector<std::string> GetTargetNames() const;
	/// <summary>
	/// 追従対象を切り替える（duration 秒かけてイージング。0 なら即座）。
	/// </summary>
	void SetTarget(const std::string& name, float duration = 0.6f, Easing::Type ease = Easing::Type::InOutCubic);
	const std::string& GetTarget() const { return targetName_; }

	// --- 視点 ---

	void SetViewMode(ViewMode mode, float duration = 0.6f, Easing::Type ease = Easing::Type::InOutCubic);
	ViewMode GetViewMode() const { return viewMode_; }
	Preset& GetPreset(ViewMode mode) { return presets_[static_cast<size_t>(mode)]; }
	static const char* ViewModeName(ViewMode mode);

	// --- ロックオン ---

	/// <summary>
	/// 第2のターゲット（敵など）を自分と一緒に画面に収める。カメラは「自分の後ろから敵の方向」を向く。
	/// </summary>
	void LockOn(const std::string& name, float duration = 0.4f, Easing::Type ease = Easing::Type::OutCubic);
	void ClearLockOn(float duration = 0.4f, Easing::Type ease = Easing::Type::OutCubic);
	bool IsLockedOn() const { return !lockOnName_.empty(); }
	const std::string& GetLockOnTarget() const { return lockOnName_; }
	float lockOnLookWeight = 0.4f;      // 注視点をどれだけ敵側に寄せるか（0 = 自分、1 = 敵）
	float lockOnExtraDistance = 0.15f;  // 敵との距離に応じてカメラを引く割合
	float lockOnMaxExtraDistance = 6.0f;

	// --- シェイク ---

	/// <summary>
	/// 揺らす（strength 0〜1 を足す。強いほど大きく、時間で収まる）。
	/// </summary>
	void Shake(float strength);
	float shakeMaxOffset = 0.35f;  // 一番強い時の位置のずれ
	float shakeFrequency = 18.0f;  // 揺れの速さ
	float shakeDecay = 1.6f;       // 1 秒あたりに弱まる量

	// --- FOV ---

	/// <summary>
	/// 一瞬だけ画角を広げて戻す（ダッシュ・被弾の演出）。addDeg 度広げ、duration 秒で戻る。
	/// </summary>
	void KickFov(float addDeg, float duration = 0.35f, Easing::Type ease = Easing::Type::OutCubic);
	/// <summary>
	/// 画角の上乗せ（速度に応じて広げる等）。毎フレーム呼んでよい。なめらかに追いかける。
	/// </summary>
	void SetFovOffset(float deg) { fovOffsetTarget_ = deg; }

	// --- キーフレーム（カットシーン）---

	/// <summary>
	/// Timeline で作ったカメラのクリップ（camera:eye / camera:lookAt / camera:fov）を再生する。
	/// 再生中は追従より優先。終わると blendOut 秒かけて追従に戻る（loop ならずっと再生）。
	/// </summary>
	void PlayClip(const KeyframeClip* clip, float blendIn = 0.5f, float blendOut = 0.5f, Easing::Type ease = Easing::Type::InOutCubic);
	void StopClip(float blendOut = 0.5f, Easing::Type ease = Easing::Type::InOutCubic);
	bool IsPlayingClip() const { return clip_ != nullptr; }

	// --- 手動（エディタの Timeline 用）---

	/// <summary>
	/// カメラの位置・注視点・画角を直接決める（追従を止める）。ReleaseManual で追従に戻る。
	/// </summary>
	void SetManualView(const View& view);
	void ReleaseManual(float duration = 0.5f, Easing::Type ease = Easing::Type::InOutCubic);
	bool IsManual() const { return manual_; }

	// --- 更新・取得 ---

	/// <summary>
	/// ターゲットを動かした後、カメラを使うオブジェクトの Update の前に毎フレーム呼ぶ。
	/// </summary>
	void Update(float deltaTime);
	Camera* GetCamera() { return &camera_; }
	/// <summary>
	/// 揺れ・FOV 演出を足す前の見え方。
	/// </summary>
	const View& GetView() const { return current_; }

	// --- 保存・ImGui ---

	bool LoadPresets(const std::string& path, bool logIfMissing = true);
	bool SavePresets(const std::string& path) const;
	/// <summary>
	/// ImGui の操作画面（追従対象・視点・ロックオン・シェイク・FOV・パラメータ）。
	/// あわせて Hierarchy / Timeline に "Camera" として登録する（Timeline でカメラのキーを打てる）。
	/// </summary>
	void DrawImGui(const char* windowName = "UniversalCamera");

private:
	struct Target {
		std::string name;
		std::function<Math::Vector3()> getPosition;
		std::function<float()> getYaw;
		Math::Vector3 lastPosition{};
		float lastYaw = 0.0f;
	};
	Target* FindTarget(const std::string& name);
	// 追従時の理想の見え方
	View ComputeFollowView(float deltaTime);
	// 今の見え方から duration 秒かけて新しい見え方へ移る
	void StartBlend(float duration, Easing::Type ease);
	void ApplyToCamera(const View& view, float extraFovDeg, const Math::Vector3& shakeOffset);

	Camera camera_;
	std::vector<Target> targets_;
	std::string targetName_;
	std::string lockOnName_;
	ViewMode viewMode_ = ViewMode::Behind;
	std::array<Preset, static_cast<size_t>(ViewMode::Count)> presets_{};

	View current_{};       // 実際に使っている見え方
	View smoothed_{};      // 追従の遅れを付けた見え方
	bool smoothedValid_ = false;
	float smoothedYaw_ = 0.0f;

	// 切り替えのイージング
	View blendFrom_{};
	float blendTime_ = 0.0f;
	float blendDuration_ = 0.0f;
	Easing::Type blendEase_ = Easing::Type::InOutCubic;

	// シェイク
	float trauma_ = 0.0f;
	float shakeTime_ = 0.0f;

	// FOV
	float fovKick_ = 0.0f;
	float fovKickTime_ = 0.0f;
	float fovKickDuration_ = 0.0f;
	Easing::Type fovKickEase_ = Easing::Type::OutCubic;
	float fovOffset_ = 0.0f;
	float fovOffsetTarget_ = 0.0f;

	// キーフレーム
	const KeyframeClip* clip_ = nullptr;
	KeyframePlayer clipPlayer_;
	float clipBlendOut_ = 0.5f;
	Easing::Type clipEase_ = Easing::Type::InOutCubic;

	// 手動
	bool manual_ = false;
	View manualView_{};

	// ImGui
	float uiDuration_ = 0.6f;
	int uiEase_ = static_cast<int>(Easing::Type::InOutCubic);
	char uiPresetPath_[260] = "Resources/Camera/presets.json";
};

} // namespace TuboEngine
