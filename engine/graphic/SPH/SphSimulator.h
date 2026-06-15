#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "Camera.h"
#include "Matrix.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Material.h"
#include "VertexData.h"
#include "SphParticle.h"
#include "SphComputePipeline.h"
#include "SphFluidRenderer.h"
#include "InstancedMeshRenderer.h"
#include "LineManager.h"
#include <string>
#include <vector>
#undef min
#undef max

/// @brief SDF 障害物 / コンテナの記述 (CPU 側インターフェース)
struct SdfObstacle {
    enum class Type {
        // ---- 障害物 (流体を外へ押し出す) ----
        Sphere = 0, Box = 1,
        // ---- コンテナ (流体を内へ押し戻す / 任意形状の水槽) ----
        SphereContainer = 2, BoxContainer = 3, CylinderContainer = 4
    } type;
    TuboEngine::Math::Vector3 center;
    // Sphere/SphereContainer : x = radius
    // Box/BoxContainer       : xyz = half-sizes
    // CylinderContainer      : x = radius, y = half-height
    TuboEngine::Math::Vector3 halfExtents;
    std::string label;

    // ---- 剛体物理 (obstacle のみ有効) ----
    bool    dynamic  = false;
    float   mass     = 1.0f;
    TuboEngine::Math::Vector3 velocity = {};

    bool IsContainer() const {
        return type == Type::SphereContainer
            || type == Type::BoxContainer
            || type == Type::CylinderContainer;
    }
};

/// @brief SPH 流体シミュレーター (GPU Compute 版)
///
/// - 物理計算: 4 本の Compute Shader が全て GPU 上で実行
/// - CPU は初期化・パラメーター更新のみ
/// - 描画: UV 球メッシュ × GPU インスタンシング → 1 DrawCall
///         ParticlePSO パス (ParticleDraw) 内で Draw() を呼ぶこと
class SphSimulator {
public:
    struct Params {
        int   particleCount   = 10000;          // 粒子数
        float smoothingRadius = 1.0f;           // 影響半径 h
        float restDensity     = 6.0f;           // 静止密度 (カーネル積分から計算した実際の値)
        float stiffness       = 200.0f;         // 圧力剛性 (Tait方程式用)
        float viscosity       = 10.0f;           // 粘性係数 (水は低粘性)
        float particleMass    = 1.0f;           // 粒子質量
        float gravity         = -9.8f;          // 重力
        float restitution     = 0.02f;          // 壁反発係数 (液体は跳ねない)
        float particleRadius  = 0.3f;          // 描画半径
        float speedMax        = 5.0f;           // 色補間の最大速度
        float xsphCoeff       = 0.15f;          // XSPH 速度補正係数 (0=無効, 0.1〜0.3が標準)
        float surfaceTension  = 2.0f;           // 表面張力係数 (0=無効, 大きいほど水滴が丸まる)
        // ---- 外力 (力点から放射状に押す/引く) ----
        bool  extForceActive   = false;
        TuboEngine::Math::Vector3 extForcePos = {0.0f, 5.0f, 0.0f};
        float extForceRadius   = 4.0f;
        float extForceStrength = 50.0f;         // 正=押し出し, 負=引き寄せ
        // ボックスは流体体積(N*m/ρ₀≈1667unit³)の3倍以上必要
        // 16×20×16=5120unit³ → 流体が底部1/3を満たす水槽になる
        TuboEngine::Math::Vector3 boundMin = {-8.0f,  0.0f, -8.0f};
        TuboEngine::Math::Vector3 boundMax = { 8.0f, 20.0f,  8.0f};
        TuboEngine::Math::Vector4 colorLow  = {0.4f, 0.7f, 1.0f, 1.0f};
        TuboEngine::Math::Vector4 colorHigh = {0.0f, 0.2f, 1.0f, 1.0f};
        int   substeps        = 3;
    };

    /// @param modelPath  球モデルのパス (デフォルト: Sphere.obj)
    /// @param texture    パーティクル PSO 用テクスチャ (Object3d 使用時は無視)
    void Initialize(const Params& params = {},
                    TuboEngine::Camera* camera = nullptr,
                    const std::string& modelPath = "Resources/Model/Sphere/Sphere.obj",
                    const std::string& texture   = "particle.png");
    void Update(float dt, TuboEngine::Camera* camera);
    void Draw();       // Object3DDraw() パス内で呼ぶ (InstancedMeshRenderer 使用)

    /// SSFR で流体を描画する (Draw() の代わりに呼ぶ)
    /// @param targetRTV  合成先の RTV (例: OffScreenRendering::GetOffscreenRtvHandle())
    /// @param targetDSV  合成先の DSV (例: DirectXCommon::GetDSVCPUDescriptorHandle(0))
    void DrawFluid(D3D12_CPU_DESCRIPTOR_HANDLE targetRTV,
                   D3D12_CPU_DESCRIPTOR_HANDLE targetDSV);

    void DrawBounds(const TuboEngine::Math::Vector4& color = {0.3f, 0.8f, 1.0f, 1.0f});
    void DrawImGui();
    void Reset();
    void Finalize();

    // ---- SDF 障害物 ----
    void AddSphere(const TuboEngine::Math::Vector3& center, float radius,
                   const std::string& label = "");
    void AddBox(const TuboEngine::Math::Vector3& center,
                const TuboEngine::Math::Vector3& halfExtents,
                const std::string& label = "");
    void AddDynamicSphere(const TuboEngine::Math::Vector3& center, float radius,
                          float mass, const std::string& label = "");
    void AddDynamicBox(const TuboEngine::Math::Vector3& center,
                       const TuboEngine::Math::Vector3& halfExtents,
                       float mass, const std::string& label = "");

    // ---- SDF コンテナ (任意形状の水槽) ----
    /// 球形の水槽。AABB は自動で合わせる
    void AddContainerSphere(const TuboEngine::Math::Vector3& center, float radius,
                            const std::string& label = "");
    /// 箱形の水槽
    void AddContainerBox(const TuboEngine::Math::Vector3& center,
                         const TuboEngine::Math::Vector3& halfExtents,
                         const std::string& label = "");
    /// 円柱形の水槽 (Y 軸方向)
    void AddContainerCylinder(const TuboEngine::Math::Vector3& center,
                              float radius, float halfHeight,
                              const std::string& label = "");

    void ClearObstacles();

    Params& GetParams() { return params_; }

    // SSFR レンダラーへのアクセス (パラメータ調整用)
    SphFluidRenderer& GetFluidRenderer() { return fluidRenderer_; }

    // プリセット (水/ハチミツ/スライム をワンクリック適用)
    enum class Preset { Water, Honey, Slime };
    void ApplyPreset(Preset preset);

private:
    // 初期グリッド配置を CPU で生成して GPU にアップロード
    std::vector<SphParticle> GenerateInitialParticles() const;
    void UpdateMouseForce();      // マウスドラッグで外力点を操作
    void IntegrateRigidBodies(float dt);  // 剛体の物理積分 (浮力・抵抗・重力)

    // 描画用 UV 球メッシュ
    void BuildGeometry();
    void EnsureRenderBuffers();

    // ---- データ ----
    Params                  params_;
    SphComputePipeline      compute_;      // GPU Compute 管理

    TuboEngine::Math::Matrix4x4 viewProj_ = {};
    TuboEngine::Math::Matrix4x4 view_     = {};  // SSFR 用 (view のみ)
    TuboEngine::Math::Matrix4x4 proj_     = {};  // SSFR 用 (proj のみ)

    InstancedMeshRenderer renderer_;   // Object3d パイプラインで 1 DrawCall 描画
    SphFluidRenderer      fluidRenderer_;  // SSFR

    // ---- SDF 障害物 ----
    std::vector<SdfObstacle> obstacles_;

    // ---- 再生コントロール ----
    bool  paused_            = false;
    bool  stepOnce_          = false;
    float timeScale_         = 1.0f;
    // ---- マウス外力 ----
    bool  mouseForceEnabled_ = false;
    bool  mouseDriving_      = false;

    // ---- ImGui 追加フォーム用 UI 状態 ----
    struct ImGuiState {
        float sphCenter[3]    = {0.f, 5.f, 0.f};
        float sphRadius       = 2.0f;
        float boxCenter[3]    = {0.f, 5.f, 0.f};
        float boxHalf[3]      = {2.f, 2.f, 2.f};
        float cntSphCenter[3] = {0.f, 8.f, 0.f};
        float cntSphRadius    = 7.0f;
        float cntBoxCenter[3] = {0.f, 8.f, 0.f};
        float cntBoxHalf[3]   = {6.f, 8.f, 6.f};
        float cntCylCenter[3] = {0.f, 8.f, 0.f};
        float cntCylRadius    = 6.0f;
        float cntCylHalfH     = 8.0f;
    } ui_;
};
