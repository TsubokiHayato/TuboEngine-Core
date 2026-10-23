#include "AnimatorController.h"
#include "Animator.h"
#include "GameTime.h"
#include "JsonManager.h"
#include "Logger.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif // USE_IMGUI
#include <cmath>
#include <format>

// =============================================================================
//  JSON の形式（LoadFromJson / SaveToJson）
//
//  {
//    "defaultState": "Idle",
//    "defaultBlendTime": 0.2,
//    "parameters": { "speed": 0.0, "sneak": false },          ← 初期値（省略可）
//    "states": [
//      { "name": "Idle", "animation": "Idle", "loop": true, "speed": 1.0 },
//      { "name": "Walk", "animation": "Walk",
//        "events": [ { "time": 0.3, "name": "footstep" } ] }  ← 省略可
//    ],
//    "transitions": [
//      { "from": "Idle", "to": "Walk", "blendTime": 0.2,
//        "conditions": [ { "param": "speed", "op": ">", "value": 0.1 } ] },
//      { "from": "*", "to": "Attack", "conditions": [ { "param": "attack", "op": "trigger" } ] },
//      { "from": "Attack", "to": "Idle", "exitTime": 1.0 }     ← 条件なし＝最後まで再生したら
//    ]
//  }
//  op: ">", ">=", "<", "<=", "==", "!=", "true", "false", "trigger"
// =============================================================================

namespace TuboEngine {

using Json = JsonManager::Json;

namespace {
constexpr const char* kAnyState = "*";
} // namespace

void AnimatorController::AddState(const std::string& name, const std::string& animation, bool loop, float speed) {
	if (!states_.contains(name)) {
		stateOrder_.push_back(name);
	}
	State& state = states_[name];
	state.name = name;
	state.animation = animation;
	state.loop = loop;
	state.speed = speed;
	if (defaultState_.empty()) {
		defaultState_ = name;
	}
	eventsBoundTo_ = nullptr; // 定義が変わったのでイベントを登録し直す
}

void AnimatorController::AddTransition(const Transition& transition) { transitions_.push_back(transition); }

void AnimatorController::AddTransition(const std::string& from, const std::string& to, const std::vector<Condition>& conditions, float blendTime, float exitTime) {
	AddTransition(Transition{from, to, conditions, blendTime, exitTime});
}

void AnimatorController::AddStateEvent(const std::string& stateName, float time, const std::string& eventName) {
	auto it = states_.find(stateName);
	if (it == states_.end()) {
		Logger::Log(std::format("[AnimatorController] 失敗: AddStateEvent の状態 '{}' がありません\n", stateName));
		return;
	}
	it->second.events.push_back({time, eventName});
	eventsBoundTo_ = nullptr;
}

void AnimatorController::Clear() {
	states_.clear();
	stateOrder_.clear();
	transitions_.clear();
	params_.clear();
	triggers_.clear();
	defaultState_.clear();
	currentState_.clear();
	forcedState_.clear();
	stateTime_ = 0.0f;
	started_ = false;
	eventsBoundTo_ = nullptr;
}

float AnimatorController::GetFloat(const std::string& name) const {
	auto it = params_.find(name);
	return it != params_.end() ? it->second : 0.0f;
}

const AnimatorController::State* AnimatorController::FindState(const std::string& name) const {
	auto it = states_.find(name);
	return it != states_.end() ? &it->second : nullptr;
}

void AnimatorController::ForceState(const std::string& name, float blendTime) {
	forcedState_ = name;
	forcedBlendTime_ = blendTime;
}

bool AnimatorController::IsSatisfied(const Transition& transition, const Animator& animator) const {
	// exitTime: 遷移元をその割合まで再生したか（ループしていても「この状態に入ってからの時間」で数える）
	if (transition.exitTime >= 0.0f) {
		const State* state = FindState(currentState_);
		const float duration = animator.GetDuration();
		const float speed = state ? std::fabs(state->speed) : 1.0f;
		const bool reached = duration <= 0.0f || animator.IsFinished() || stateTime_ * speed >= transition.exitTime * duration;
		if (!reached) {
			return false;
		}
	}
	for (const Condition& c : transition.conditions) {
		const float v = GetFloat(c.param);
		bool ok = false;
		switch (c.op) {
		case Op::Greater:      ok = v > c.value; break;
		case Op::GreaterEqual: ok = v >= c.value; break;
		case Op::Less:         ok = v < c.value; break;
		case Op::LessEqual:    ok = v <= c.value; break;
		case Op::Equal:        ok = std::fabs(v - c.value) < 1e-4f; break;
		case Op::NotEqual:     ok = std::fabs(v - c.value) >= 1e-4f; break;
		case Op::True:         ok = v != 0.0f; break;
		case Op::False:        ok = v == 0.0f; break;
		case Op::Trigger:      ok = triggers_.contains(c.param); break;
		}
		if (!ok) {
			return false;
		}
	}
	return true;
}

void AnimatorController::EnterState(Animator& animator, const std::string& name, float blendTime) {
	const State* state = FindState(name);
	if (!state) {
		Logger::Log(std::format("[AnimatorController] 失敗: 状態 '{}' がありません\n", name));
		return;
	}
	animator.SetLoop(state->loop);
	animator.SetSpeed(state->speed);
	if (!animator.PlayAnimation(state->animation, blendTime < 0.0f ? defaultBlendTime_ : blendTime)) {
		Logger::Log(std::format("[AnimatorController] 失敗: アニメーション '{}' が Animator に登録されていません（状態 '{}'）\n", state->animation, name));
	}
	currentState_ = name;
	stateTime_ = 0.0f;
}

void AnimatorController::Update(Animator& animator) {
	// 状態のイベントを Animator に登録する（初回・定義を変えた後・別の Animator に使われた時）
	if (eventsBoundTo_ != &animator) {
		for (const auto& [name, state] : states_) {
			if (!state.events.empty()) {
				animator.ClearEvents(state.animation);
			}
		}
		for (const auto& [name, state] : states_) {
			for (const State::Event& e : state.events) {
				animator.AddEvent(state.animation, e.time, nullptr, e.name);
			}
		}
		eventsBoundTo_ = &animator;
	}

	// 初回は開始状態へ（切り替え時間なし）
	if (!started_) {
		started_ = true;
		if (!defaultState_.empty()) {
			EnterState(animator, defaultState_, 0.0f);
		}
		return;
	}

	stateTime_ += Time::DeltaTime();

	if (!forcedState_.empty()) {
		const std::string next = forcedState_;
		forcedState_.clear();
		EnterState(animator, next, forcedBlendTime_);
		return;
	}

	// "*"（どこからでも）の遷移を先に、次に今の状態からの遷移を、追加した順に調べる
	for (int pass = 0; pass < 2; ++pass) {
		for (const Transition& t : transitions_) {
			const bool isAny = t.from == kAnyState;
			if ((pass == 0) != isAny) {
				continue;
			}
			if (!isAny && t.from != currentState_) {
				continue;
			}
			if (t.to == currentState_) {
				continue; // 自分自身へは遷移しない（"*" で毎フレーム再生し直さないように）
			}
			if (!IsSatisfied(t, animator)) {
				continue;
			}
			// 使ったトリガーは消す
			for (const Condition& c : t.conditions) {
				if (c.op == Op::Trigger) {
					triggers_.erase(c.param);
				}
			}
			EnterState(animator, t.to, t.blendTime);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// JSON
// -----------------------------------------------------------------------------
const char* AnimatorController::OpToString(Op op) {
	switch (op) {
	case Op::Greater:      return ">";
	case Op::GreaterEqual: return ">=";
	case Op::Less:         return "<";
	case Op::LessEqual:    return "<=";
	case Op::Equal:        return "==";
	case Op::NotEqual:     return "!=";
	case Op::True:         return "true";
	case Op::False:        return "false";
	case Op::Trigger:      return "trigger";
	}
	return "true";
}

bool AnimatorController::OpFromString(const std::string& text, Op& out) {
	static const std::pair<const char*, Op> kTable[] = {
	    {">", Op::Greater}, {">=", Op::GreaterEqual}, {"<", Op::Less},   {"<=", Op::LessEqual},     {"==", Op::Equal},
	    {"!=", Op::NotEqual}, {"true", Op::True},     {"false", Op::False}, {"trigger", Op::Trigger},
	};
	for (const auto& [name, op] : kTable) {
		if (text == name) {
			out = op;
			return true;
		}
	}
	return false;
}

bool AnimatorController::LoadFromJson(const std::string& path) {
	Json root;
	if (!JsonManager::Load(path, root)) {
		return false;
	}
	if (!root.is_object()) {
		Logger::Log(std::format("[AnimatorController] 失敗: {} の中身がオブジェクトではありません\n", path));
		return false;
	}

	Clear();
	JsonManager::Read(root, "defaultBlendTime", defaultBlendTime_);

	if (root.contains("parameters") && root["parameters"].is_object()) {
		for (const auto& [name, value] : root["parameters"].items()) {
			if (value.is_boolean()) {
				SetBool(name, value.get<bool>());
			} else if (value.is_number()) {
				SetFloat(name, value.get<float>());
			}
		}
	}

	if (root.contains("states") && root["states"].is_array()) {
		for (const Json& js : root["states"]) {
			std::string name, animation;
			bool loop = true;
			float speed = 1.0f;
			JsonManager::Read(js, "name", name);
			JsonManager::Read(js, "animation", animation);
			JsonManager::Read(js, "loop", loop);
			JsonManager::Read(js, "speed", speed);
			if (name.empty()) {
				continue;
			}
			AddState(name, animation.empty() ? name : animation, loop, speed);
			if (js.contains("events") && js["events"].is_array()) {
				for (const Json& je : js["events"]) {
					State::Event e;
					JsonManager::Read(je, "time", e.time);
					JsonManager::Read(je, "name", e.name);
					states_[name].events.push_back(e);
				}
			}
		}
	}

	if (root.contains("transitions") && root["transitions"].is_array()) {
		for (const Json& jt : root["transitions"]) {
			Transition t;
			JsonManager::Read(jt, "from", t.from);
			JsonManager::Read(jt, "to", t.to);
			JsonManager::Read(jt, "blendTime", t.blendTime);
			JsonManager::Read(jt, "exitTime", t.exitTime);
			if (jt.contains("conditions") && jt["conditions"].is_array()) {
				for (const Json& jc : jt["conditions"]) {
					Condition c;
					std::string op = "true";
					JsonManager::Read(jc, "param", c.param);
					JsonManager::Read(jc, "op", op);
					JsonManager::Read(jc, "value", c.value);
					if (!OpFromString(op, c.op)) {
						Logger::Log(std::format("[AnimatorController] 失敗: {} の op '{}' が分かりません\n", path, op));
						continue;
					}
					t.conditions.push_back(c);
				}
			}
			if (t.from.empty() || t.to.empty()) {
				continue;
			}
			transitions_.push_back(t);
		}
	}

	// defaultState が無ければ最初の状態（AddState が設定済み）
	std::string defaultState;
	if (JsonManager::Read(root, "defaultState", defaultState) && !defaultState.empty()) {
		defaultState_ = defaultState;
	}
	return true;
}

bool AnimatorController::SaveToJson(const std::string& path) const {
	Json root;
	root["defaultState"] = defaultState_;
	root["defaultBlendTime"] = defaultBlendTime_;
	root["parameters"] = Json::object();
	for (const auto& [name, value] : params_) {
		root["parameters"][name] = value;
	}
	root["states"] = Json::array();
	for (const std::string& name : stateOrder_) {
		const State& s = states_.at(name);
		Json js{{"name", s.name}, {"animation", s.animation}, {"loop", s.loop}, {"speed", s.speed}};
		if (!s.events.empty()) {
			js["events"] = Json::array();
			for (const State::Event& e : s.events) {
				js["events"].push_back({{"time", e.time}, {"name", e.name}});
			}
		}
		root["states"].push_back(js);
	}
	root["transitions"] = Json::array();
	for (const Transition& t : transitions_) {
		Json jt{{"from", t.from}, {"to", t.to}, {"blendTime", t.blendTime}, {"exitTime", t.exitTime}};
		jt["conditions"] = Json::array();
		for (const Condition& c : t.conditions) {
			jt["conditions"].push_back({{"param", c.param}, {"op", OpToString(c.op)}, {"value", c.value}});
		}
		root["transitions"].push_back(jt);
	}
	return JsonManager::Save(path, root);
}

// -----------------------------------------------------------------------------
// ImGui
// -----------------------------------------------------------------------------
void AnimatorController::DrawImGui(const char* windowName) {
#ifdef USE_IMGUI
	ImGui::Begin(windowName);
	if (ImGui::CollapsingHeader("Animator Controller", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("State: %s  (%.2f s)", currentState_.empty() ? "(none)" : currentState_.c_str(), stateTime_);

		// 状態をクリックで強制的に切り替え
		for (const std::string& name : stateOrder_) {
			const bool isCurrent = name == currentState_;
			if (isCurrent) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			if (ImGui::SmallButton(name.c_str())) {
				ForceState(name);
			}
			if (isCurrent) {
				ImGui::PopStyleColor();
			}
			ImGui::SameLine();
		}
		ImGui::NewLine();

		ImGui::SeparatorText("Parameters");
		for (auto& [name, value] : params_) {
			ImGui::DragFloat(name.c_str(), &value, 0.01f);
		}
		if (!triggers_.empty()) {
			ImGui::Text("Triggers:");
			for (const auto& [name, set] : triggers_) {
				ImGui::SameLine();
				ImGui::Text("%s", name.c_str());
			}
		}

		if (ImGui::TreeNode("Transitions")) {
			for (const Transition& t : transitions_) {
				std::string text = t.from + " -> " + t.to;
				for (const Condition& c : t.conditions) {
					text += std::format("  [{} {} {:.2f}]", c.param, OpToString(c.op), c.value);
				}
				if (t.exitTime >= 0.0f) {
					text += std::format("  exit={:.2f}", t.exitTime);
				}
				ImGui::BulletText("%s", text.c_str());
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
#else
	(void)windowName;
#endif // USE_IMGUI
}

} // namespace TuboEngine
