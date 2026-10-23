#pragma once
#include <map>
#include <string>
#include <vector>

namespace TuboEngine {

class Animator;

/// <summary>
/// Unity の Animator Controller のような、アニメーションの状態遷移を管理するクラス。
/// 「状態（State）」と「遷移（Transition）＋条件」を登録しておき、パラメータを渡すだけで
/// 条件に合ったアニメーションへ自動で切り替える（シーン側に if を並べなくてよい）。
/// </summary>
/// <remarks>
/// 使い方（コードで定義）:
///   controller_.AddState("Idle", "Idle");
///   controller_.AddState("Walk", "Walk");
///   controller_.AddTransition("Idle", "Walk", {{"speed", AnimatorController::Op::Greater, 0.1f}});
///   controller_.AddTransition("Walk", "Idle", {{"speed", AnimatorController::Op::LessEqual, 0.1f}});
///   controller_.SetDefaultState("Idle");
///   // Update 内
///   controller_.SetFloat("speed", moveSpeed);
///   controller_.Update(*animator_);   // ← animator_->Update() より前に呼ぶ
///   animator_->Update();
///
/// JSON で定義する場合は LoadFromJson（形式は AnimatorController.cpp 冒頭のコメント）。
/// </remarks>
class AnimatorController {
public:
	/// <summary>
	/// 条件の比較方法。
	/// </summary>
	enum class Op {
		Greater,      // >
		GreaterEqual, // >=
		Less,         // <
		LessEqual,    // <=
		Equal,        // ==
		NotEqual,     // !=
		True,         // bool が true（value は使わない）
		False,        // bool が false
		Trigger,      // SetTrigger されていたら成立（遷移したら自動で消える）
	};

	/// <summary>
	/// 遷移の条件1つ（パラメータ param を value と比べる）。
	/// </summary>
	struct Condition {
		std::string param;
		Op op = Op::True;
		float value = 0.0f;
	};

	/// <summary>
	/// 状態1つ。animation は Animator に登録したアニメーション名。
	/// </summary>
	struct State {
		std::string name;
		std::string animation;
		bool loop = true;
		float speed = 1.0f;
		// この状態のアニメーションに付けるイベント（Animator::SetEventListener に名前が届く）
		struct Event {
			float time = 0.0f;
			std::string name;
		};
		std::vector<Event> events;
	};

	/// <summary>
	/// 遷移1つ。conditions が全部成立したら to へ切り替える。
	/// </summary>
	struct Transition {
		std::string from;                 // 遷移元の状態名。"*" ならどの状態からでも（Any State）
		std::string to;                   // 遷移先の状態名
		std::vector<Condition> conditions;
		float blendTime = -1.0f;          // 切り替えにかける秒数。負なら SetDefaultBlendTime の値
		float exitTime = -1.0f;           // 0〜1。0 以上なら「遷移元をその割合まで再生してから」切り替える（1 = 最後まで）
	};

	// --- 定義 ---

	/// <summary>
	/// 状態を追加する（同名は上書き）。最初に追加した状態が既定の開始状態になる。
	/// </summary>
	void AddState(const std::string& name, const std::string& animation, bool loop = true, float speed = 1.0f);
	/// <summary>
	/// 遷移を追加する。同じ状態からの遷移は「追加した順」に調べ、最初に成立したものを使う。
	/// "*"（どこからでも）の遷移は、通常の遷移より先に調べる。
	/// </summary>
	void AddTransition(const Transition& transition);
	void AddTransition(const std::string& from, const std::string& to, const std::vector<Condition>& conditions, float blendTime = -1.0f, float exitTime = -1.0f);
	/// <summary>
	/// 状態のアニメーションにイベントを付ける（time 秒を通過すると Animator::SetEventListener に eventName が届く）。
	/// ※ 最初の Update で Animator に登録される。同じアニメーションに Animator::AddEvent で付けたイベントとは混ぜないこと
	///   （登録し直す時に、そのアニメーションのイベントを一度消すため）。
	/// </summary>
	void AddStateEvent(const std::string& stateName, float time, const std::string& eventName);
	/// <summary>
	/// 開始状態を指定する。
	/// </summary>
	void SetDefaultState(const std::string& name) { defaultState_ = name; }
	/// <summary>
	/// blendTime を省略した遷移の切り替え秒数。
	/// </summary>
	void SetDefaultBlendTime(float seconds) { defaultBlendTime_ = seconds; }
	/// <summary>
	/// 状態・遷移・パラメータを全て消す。
	/// </summary>
	void Clear();

	/// <summary>
	/// JSON ファイルから定義を読み込む（今の定義は消える）。失敗したら false（理由は Console）。
	/// </summary>
	bool LoadFromJson(const std::string& path);
	/// <summary>
	/// 今の定義を JSON ファイルに保存する。
	/// </summary>
	bool SaveToJson(const std::string& path) const;

	// --- パラメータ（毎フレーム、シーンから値を渡す）---

	void SetFloat(const std::string& name, float value) { params_[name] = value; }
	void SetBool(const std::string& name, bool value) { params_[name] = value ? 1.0f : 0.0f; }
	void SetInt(const std::string& name, int value) { params_[name] = static_cast<float>(value); }
	/// <summary>
	/// 1回だけ成立する条件（攻撃ボタンなど）。Op::Trigger の遷移で使われたら自動で消える。
	/// </summary>
	void SetTrigger(const std::string& name) { triggers_[name] = true; }
	void ResetTrigger(const std::string& name) { triggers_.erase(name); }
	float GetFloat(const std::string& name) const;
	bool GetBool(const std::string& name) const { return GetFloat(name) != 0.0f; }

	// --- 実行 ---

	/// <summary>
	/// 遷移の条件を調べて、必要なら animator のアニメーションを切り替える。
	/// animator.Update() の前に毎フレーム呼ぶ。初回は開始状態を再生する。
	/// </summary>
	void Update(Animator& animator);
	/// <summary>
	/// 条件に関係なく、指定の状態へ切り替える（次の Update で反映）。
	/// </summary>
	void ForceState(const std::string& name, float blendTime = -1.0f);
	/// <summary>
	/// 今の状態名（Update する前は空）。
	/// </summary>
	const std::string& GetCurrentState() const { return currentState_; }
	/// <summary>
	/// 今の状態に入ってからの秒数。
	/// </summary>
	float GetStateTime() const { return stateTime_; }

	/// <summary>
	/// ImGui に状態・パラメータ・遷移を表示する（パラメータはその場で変更できる）。
	/// </summary>
	void DrawImGui(const char* windowName);

	// 比較方法 ⇔ 文字列（JSON 用: ">", ">=", "<", "<=", "==", "!=", "true", "false", "trigger"）
	static const char* OpToString(Op op);
	static bool OpFromString(const std::string& text, Op& out);

private:
	const State* FindState(const std::string& name) const;
	bool IsSatisfied(const Transition& transition, const Animator& animator) const;
	void EnterState(Animator& animator, const std::string& name, float blendTime);

	std::map<std::string, State> states_;
	std::vector<std::string> stateOrder_; // 追加順（ImGui 表示・JSON 保存用）
	std::vector<Transition> transitions_;
	std::map<std::string, float> params_;
	std::map<std::string, bool> triggers_;
	std::string defaultState_;
	std::string currentState_;
	std::string forcedState_;
	float forcedBlendTime_ = -1.0f;
	float defaultBlendTime_ = 0.2f;
	float stateTime_ = 0.0f;
	bool started_ = false;
	// 状態のイベントを登録済みの Animator（別の Animator に使われたら登録し直す）
	const Animator* eventsBoundTo_ = nullptr;
};

} // namespace TuboEngine
