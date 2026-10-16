#pragma once
#include "Animation.h"
#include "Object3d.h"
#include "Skeleton.h"
#include "SkinCluster.h"
#include <map>
#include <memory>
#include <string>

namespace TuboEngine {
/// <summary>
/// アニメーション付き3Dモデルの再生と描画を行うクラス。
/// ボーン（スキン）を持つモデルはスキニング、持たないモデルはノードアニメーションで動かす。
/// </summary>
/// <remarks>
/// スキニングするモデル（IsSkinned() == true）は、描画前に
/// Object3dCommon::DrawSettingsSkinning() を呼んでおくこと。
/// ノードアニメーションのみのモデルは通常の Object3dCommon::DrawSettingsCommon() で描画できる。
/// </remarks>
class Animator {
public:
	/// <summary>
	/// 初期化。モデルを読み込み、同じファイルに含まれる全アニメーションを登録して先頭を再生する。
	/// </summary>
	/// <param name="modelFileNamePath">Resources/Models からの相対パス（例: "human/walk.gltf"）</param>
	void Initialize(const std::string& modelFileNamePath);

	/// <summary>
	/// 更新処理（アニメーションを進めて行列を更新する）
	/// </summary>
	/// <param name="deltaTime">経過時間（秒）</param>
	void Update(float deltaTime = 1.0f / 60.0f);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// ImGuiで再生状態・トランスフォーム・ライトを操作する
	/// </summary>
	void DrawImGui(const char* windowName);

public:
	///-------------------------------------------------------------------------------------------------
	/// アニメーション制御

	/// <summary>
	/// 登録済みのアニメーションに切り替える。再生中と同じ名前なら何もしない。
	/// </summary>
	/// <param name="name">アニメーション名（glTF のアニメーション名 / Blender のアクション名）</param>
	/// <param name="blendTime">前のアニメーションから混ぜながら切り替える時間（秒）。0 で即座に切り替え</param>
	/// <returns>見つからなければ false</returns>
	bool PlayAnimation(const std::string& name, float blendTime = 0.2f);
	/// <summary>
	/// アニメーションを名前付きで登録する（同名は上書き）。
	/// </summary>
	void AddAnimation(const std::string& name, const Animation& animation);
	/// <summary>
	/// 別ファイルのアニメーションを名前を付けて登録する（ファイル間でアニメーション名が重なるとき用）。
	/// </summary>
	/// <param name="name">登録名</param>
	/// <param name="filePath">Resources/Models からの相対パス（同じスケルトンを持つファイルであること）</param>
	/// <param name="animationIndex">ファイル内のアニメーション番号</param>
	void AddAnimation(const std::string& name, const std::string& filePath, uint32_t animationIndex = 0);
	/// <summary>
	/// 登録済みのアニメーションを取得する（無ければ nullptr）。
	/// </summary>
	const Animation* FindAnimation(const std::string& name) const;
	/// <summary>
	/// 別ファイルのアニメーションを全て読み込んで登録する（同じスケルトンを持つファイルであること）。
	/// </summary>
	/// <param name="filePath">Resources/Models からの相対パス</param>
	void LoadAnimations(const std::string& filePath);
	/// <summary>
	/// 再生するアニメーションを直接設定する（name で登録もされる）。
	/// </summary>
	void SetAnimation(const Animation& animation, bool resetTime = true);
	/// <summary>
	/// 別ファイルのアニメーションを読み込んで設定する（同じスケルトンを持つファイルであること）。
	/// </summary>
	/// <param name="filePath">Resources/Models からの相対パス</param>
	/// <param name="animationIndex">ファイル内のアニメーション番号</param>
	void LoadAnimation(const std::string& filePath, uint32_t animationIndex = 0);

	// 登録済みのアニメーション名一覧
	std::vector<std::string> GetAnimationNames() const;
	// 再生中のアニメーション名
	const std::string& GetCurrentAnimationName() const { return currentName_; }
	// クロスフェード中か
	bool IsBlending() const { return previous_ != nullptr; }

	void Play() { isPlaying_ = true; }
	void Pause() { isPlaying_ = false; }
	bool IsPlaying() const { return isPlaying_; }

	void SetLoop(bool loop) { isLoop_ = loop; }
	bool IsLoop() const { return isLoop_; }

	// 再生速度（1.0 で等速、負の値で逆再生）
	void SetSpeed(float speed) { speed_ = speed; }
	float GetSpeed() const { return speed_; }

	void SetAnimationTime(float time) { animationTime_ = time; }
	float GetAnimationTime() const { return animationTime_; }
	float GetDuration() const { return current_->duration; }
	// ループしない再生で終端に到達したか
	bool IsFinished() const { return !isLoop_ && current_->duration > 0.0f && (speed_ >= 0.0f ? animationTime_ >= current_->duration : animationTime_ <= 0.0f); }

	// スキニングで描画するか
	bool IsSkinned() const { return skinCluster_ != nullptr; }

	const Animation& GetAnimation() const { return *current_; }
	const Skeleton& GetSkeleton() const { return skeleton_; }

	///-------------------------------------------------------------------------------------------------
	/// トランスフォーム・見た目

	void SetScale(const TuboEngine::Math::Vector3& scale) { object3d_->SetScale(scale); }
	void SetRotation(const TuboEngine::Math::Vector3& rotation) { object3d_->SetRotation(rotation); }
	void SetPosition(const TuboEngine::Math::Vector3& position) { object3d_->SetPosition(position); }
	TuboEngine::Math::Vector3 GetScale() const { return object3d_->GetScale(); }
	TuboEngine::Math::Vector3 GetRotation() const { return object3d_->GetRotation(); }
	TuboEngine::Math::Vector3 GetPosition() const { return object3d_->GetPosition(); }

	void SetCamera(TuboEngine::Camera* camera) { object3d_->SetCamera(camera); }
	TuboEngine::Camera* GetCamera() const { return object3d_->GetCamera(); }

	void SetModelColor(const TuboEngine::Math::Vector4& color) { object3d_->SetModelColor(color); }
	TuboEngine::Math::Vector4 GetModelColor() { return object3d_->GetModelColor(); }

	void SetLightType(int type) { object3d_->SetLightType(type); }

	/// <summary>
	/// 内部の Object3d（ライト設定など Object3d の機能を直接使いたいとき用）
	/// </summary>
	Object3d* GetObject3d() const { return object3d_.get(); }

private:
	// 描画を担当する Object3d
	std::unique_ptr<Object3d> object3d_;
	// モデル（ModelManager が所有）
	TuboEngine::Model* model_ = nullptr;

	/// <summary>
	/// ループ・クランプを考慮して再生時間を進める
	/// </summary>
	float AdvanceTime(float time, float deltaTime, float duration) const;

	// 登録済みのアニメーション（名前 → データ）
	std::map<std::string, Animation> animations_;
	// アニメーションが無いときに指す空データ
	Animation emptyAnimation_;
	// 再生中のアニメーション（animations_ の要素か emptyAnimation_ を指す）
	const Animation* current_ = &emptyAnimation_;
	std::string currentName_;
	// クロスフェード元（ブレンド中以外は nullptr）
	const Animation* previous_ = nullptr;
	float previousTime_ = 0.0f;
	float blendTime_ = 0.0f;
	float blendElapsed_ = 0.0f;
	// モデルのノード階層から作ったスケルトン
	Skeleton skeleton_;
	// スキニング用リソース（スキンを持たないモデルは nullptr）。Object3d がポインタを保持するので unique_ptr で固定する
	std::unique_ptr<SkinCluster> skinCluster_;
	// ノードアニメーション時に行列を取り出すジョイント（メッシュを持つノード）
	int32_t meshJointIndex_ = 0;

	float animationTime_ = 0.0f; // 再生時間（秒）
	float speed_ = 1.0f;         // 再生速度
	bool isLoop_ = true;         // ループ再生
	bool isPlaying_ = true;      // 再生中
};
} // namespace TuboEngine
