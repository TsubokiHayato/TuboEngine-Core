#pragma once
#include <functional>
#include <memory>
#include <vector>

//=============================================================================
// ビヘイビアツリー (Behavior Tree) フレームワーク
//
// 【ノード種類】
//   Sequence  : 子を左から順に実行。1つでも Failure → Failure、全 Success → Success
//   Selector  : 子を左から順に実行。1つでも Success → Success、全 Failure → Failure
//   Condition : bool を返すラムダ。true → Success、false → Failure
//   Action    : NodeStatus を返すラムダ。任意の処理を記述する
//   Act       : void を返すラムダの省略形。常に Success を返す
//   Inverter  : 子の結果を反転する
//=============================================================================

namespace BT {

/// ノード実行結果
enum class NodeStatus {
    Success,  ///< 成功
    Failure,  ///< 失敗
    Running   ///< 実行中（複数フレームにまたがる処理）
};

//-----------------------------------------------------------------------------
// 基底クラス
//-----------------------------------------------------------------------------
/// <summary>
/// ビヘイビアツリーの全ノード共通の基底クラス。
/// </summary>
class BehaviorNode {
public:
    /// <summary>
    /// デストラクタ。
    /// </summary>
    virtual ~BehaviorNode() = default;
    /// 毎フレーム呼ばれる実行メソッド
    virtual NodeStatus Tick() = 0;
    /// 状態リセット（必要に応じてオーバーライド）
    virtual void Reset() {}
};

//-----------------------------------------------------------------------------
// Sequence（AND ノード）
// 子を左から順に Tick し、最初に Failure/Running が出たらそこで止まる。
// 全子が Success を返したら Success。
//-----------------------------------------------------------------------------
/// <summary>
/// Sequence（AND）ノード。子を左から順に実行し、Failure/Running が出たら停止する。
/// </summary>
class SequenceNode : public BehaviorNode {
public:
    /// <summary>
    /// 子ノードを追加する。
    /// </summary>
    void AddChild(std::unique_ptr<BehaviorNode> child) {
        children_.push_back(std::move(child));
    }

    /// <summary>
    /// ノードを実行し結果を返す。
    /// </summary>
    NodeStatus Tick() override {
        for (auto& child : children_) {
            NodeStatus s = child->Tick();
            if (s != NodeStatus::Success) return s;
        }
        return NodeStatus::Success;
    }

    /// <summary>
    /// 状態のリセット。
    /// </summary>
    void Reset() override {
        for (auto& c : children_) c->Reset();
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> children_;
};

//-----------------------------------------------------------------------------
// Selector（OR ノード / Fallback ノード）
// 子を左から順に Tick し、最初に Success/Running が出たらそこで止まる。
// 全子が Failure を返したら Failure。
//-----------------------------------------------------------------------------
/// <summary>
/// Selector（OR）ノード。子を左から順に実行し、Success/Running が出たら停止する。
/// </summary>
class SelectorNode : public BehaviorNode {
public:
    /// <summary>
    /// 子ノードを追加する。
    /// </summary>
    void AddChild(std::unique_ptr<BehaviorNode> child) {
        children_.push_back(std::move(child));
    }

    /// <summary>
    /// ノードを実行し結果を返す。
    /// </summary>
    NodeStatus Tick() override {
        for (auto& child : children_) {
            NodeStatus s = child->Tick();
            if (s != NodeStatus::Failure) return s;
        }
        return NodeStatus::Failure;
    }

    /// <summary>
    /// 状態のリセット。
    /// </summary>
    void Reset() override {
        for (auto& c : children_) c->Reset();
    }

private:
    std::vector<std::unique_ptr<BehaviorNode>> children_;
};

//-----------------------------------------------------------------------------
// Inverter（デコレータ）
// 子の Success ↔ Failure を反転する。Running はそのまま。
//-----------------------------------------------------------------------------
/// <summary>
/// 子の Success/Failure を反転するデコレータノード。
/// </summary>
class InverterNode : public BehaviorNode {
public:
    /// <summary>
    /// コンストラクタ。
    /// </summary>
    explicit InverterNode(std::unique_ptr<BehaviorNode> child)
        : child_(std::move(child)) {}

    /// <summary>
    /// ノードを実行し結果を返す。
    /// </summary>
    NodeStatus Tick() override {
        NodeStatus s = child_->Tick();
        if (s == NodeStatus::Success) return NodeStatus::Failure;
        if (s == NodeStatus::Failure) return NodeStatus::Success;
        return NodeStatus::Running;
    }

    /// <summary>
    /// 状態のリセット。
    /// </summary>
    void Reset() override { child_->Reset(); }

private:
    std::unique_ptr<BehaviorNode> child_;
};

//-----------------------------------------------------------------------------
// ActionNode（リーフ）
// NodeStatus を返すラムダを実行する。
//-----------------------------------------------------------------------------
/// <summary>
/// NodeStatus を返すラムダを実行するリーフノード。
/// </summary>
class ActionNode : public BehaviorNode {
public:
    /// <summary>
    /// コンストラクタ。
    /// </summary>
    explicit ActionNode(std::function<NodeStatus()> action)
        : action_(std::move(action)) {}

    /// <summary>
    /// ノードを実行し結果を返す。
    /// </summary>
    NodeStatus Tick() override { return action_(); }

private:
    std::function<NodeStatus()> action_;
};

//-----------------------------------------------------------------------------
// ConditionNode（リーフ）
// bool を返すラムダで条件チェック。true → Success、false → Failure。
//-----------------------------------------------------------------------------
/// <summary>
/// bool を返すラムダで条件判定するリーフノード。
/// </summary>
class ConditionNode : public BehaviorNode {
public:
    /// <summary>
    /// コンストラクタ。
    /// </summary>
    explicit ConditionNode(std::function<bool()> cond)
        : cond_(std::move(cond)) {}

    /// <summary>
    /// ノードを実行し結果を返す。
    /// </summary>
    NodeStatus Tick() override {
        return cond_() ? NodeStatus::Success : NodeStatus::Failure;
    }

private:
    std::function<bool()> cond_;
};

//=============================================================================
// ファクトリ関数（ツリー構築用ヘルパー）
//=============================================================================

/// Sequence（AND）ノード生成
inline std::unique_ptr<SequenceNode> Sequence() {
    return std::make_unique<SequenceNode>();
}

/// Selector（OR）ノード生成
inline std::unique_ptr<SelectorNode> Selector() {
    return std::make_unique<SelectorNode>();
}

/// 条件ノード生成
inline std::unique_ptr<ConditionNode> Condition(std::function<bool()> fn) {
    return std::make_unique<ConditionNode>(std::move(fn));
}

/// NodeStatus を返す汎用アクションノード生成
inline std::unique_ptr<ActionNode> Action(std::function<NodeStatus()> fn) {
    return std::make_unique<ActionNode>(std::move(fn));
}

/// void ラムダを包んで常に Success を返すアクションノード生成
inline std::unique_ptr<ActionNode> Act(std::function<void()> fn) {
    return std::make_unique<ActionNode>([fn = std::move(fn)]() -> NodeStatus {
        fn();
        return NodeStatus::Success;
    });
}

/// 子の結果を反転するデコレータノード生成
inline std::unique_ptr<InverterNode> Inverter(std::unique_ptr<BehaviorNode> child) {
    return std::make_unique<InverterNode>(std::move(child));
}

} // namespace BT
