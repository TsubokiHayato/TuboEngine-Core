#include "SphSimulator.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "WinApp.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>
#undef min
#undef max

static constexpr float kPi = std::numbers::pi_v<float>;

// 行ベクトル × 行優先行列 (このエンジンの規約: pos * matrix)
static TuboEngine::Math::Vector4 MulRowVec4(const TuboEngine::Math::Vector4& v,
                                            const TuboEngine::Math::Matrix4x4& m) {
    return {
        v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0],
        v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1],
        v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2],
        v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3],
    };
}

// ============================================================
//  Initialize
// ============================================================
void SphSimulator::Initialize(const Params& params, TuboEngine::Camera* camera,
                                const std::string& modelPath, const std::string& texture)
{
    params_ = params;

    viewProj_ = TuboEngine::Math::MakeIdentity4x4();

    // GPU Compute パイプライン (セルサイズ = smoothingRadius で空間ハッシュ構築)
    compute_.Initialize(params_.particleCount, params_.boundMin, params_.boundMax,
                        params_.smoothingRadius);

    // 初期粒子データを GPU にアップロード
    auto initialParticles = GenerateInitialParticles();
    compute_.UploadInitialParticles(initialParticles);

    // InstancedMeshRenderer — Object3d パイプライン + 1 DrawCall
    renderer_.Initialize(modelPath, params_.particleCount, camera);
    renderer_.SetLightType(1);   // Phong

    // SSFR レンダラー
    const int w = TuboEngine::WinApp::GetInstance()->GetClientWidth();
    const int h = TuboEngine::WinApp::GetInstance()->GetClientHeight();
    fluidRenderer_.Initialize(w, h);
}

// ============================================================
//  Finalize
// ============================================================
void SphSimulator::Finalize() {
    fluidRenderer_.Finalize();
    renderer_.Finalize();
    compute_.Finalize();
}

// ============================================================
//  Reset — 粒子を初期配置に戻して GPU に再アップロード
// ============================================================
void SphSimulator::Reset() {
    auto initialParticles = GenerateInitialParticles();
    compute_.UploadInitialParticles(initialParticles);
}

// ============================================================
//  Update — GPU Dispatch (CPU は ViewProj だけ渡す)
// ============================================================
void SphSimulator::Update(float dt, TuboEngine::Camera* camera) {
    if (camera) {
        viewProj_ = camera->GetViewProjectionMatrix();
        view_     = camera->GetViewMatrix();
        proj_     = camera->GetProjectionMatrix();
    }
    renderer_.Update(camera);

    // マウスドラッグで外力点を操作
    UpdateMouseForce();

    // 再生制御: 一時停止中は dt=0 (描画行列は更新される)、再生速度でスケール、
    // コマ送りは次の1フレームだけ通常 dt で進める
    float simDt = dt * timeScale_;
    if (paused_ && !stepOnce_) simDt = 0.0f;
    stepOnce_ = false;

    // GPU パラメーターを組み立て
    SphGpuParams gp{};
    gp.particleCount = params_.particleCount;
    gp.h             = params_.smoothingRadius;
    gp.restDensity   = params_.restDensity;
    gp.stiffness     = params_.stiffness;
    gp.viscosity     = params_.viscosity;
    gp.mass          = params_.particleMass;
    gp.gravity       = params_.gravity;
    gp.restitution   = params_.restitution;
    gp.dt            = simDt / float(params_.substeps);
    gp.boundMinX = params_.boundMin.x;
    gp.boundMinY = params_.boundMin.y;
    gp.boundMinZ = params_.boundMin.z;
    gp.boundMaxX = params_.boundMax.x;
    gp.boundMaxY = params_.boundMax.y;
    gp.boundMaxZ = params_.boundMax.z;
    gp.speedMax       = params_.speedMax;
    gp.colorLow[0]  = params_.colorLow.x;  gp.colorLow[1]  = params_.colorLow.y;
    gp.colorLow[2]  = params_.colorLow.z;  gp.colorLow[3]  = params_.colorLow.w;
    gp.colorHigh[0] = params_.colorHigh.x; gp.colorHigh[1] = params_.colorHigh.y;
    gp.colorHigh[2] = params_.colorHigh.z; gp.colorHigh[3] = params_.colorHigh.w;
    gp.particleRadius = params_.particleRadius;
    gp.xsphCoeff      = params_.xsphCoeff;
    gp.extForcePosX     = params_.extForcePos.x;
    gp.extForcePosY     = params_.extForcePos.y;
    gp.extForcePosZ     = params_.extForcePos.z;
    gp.extForceRadius   = params_.extForceRadius;
    gp.extForceStrength = params_.extForceStrength;
    gp.extForceActive   = params_.extForceActive ? 1 : 0;
    gp.surfaceTension   = params_.surfaceTension;
    // 剛体物理を先に積分し、障害物の位置を更新してから GPU へ送る
    IntegrateRigidBodies(simDt);

    // SDF 障害物を GPU パラメーターに転送
    gp.sdfCount = std::min((int)obstacles_.size(), kMaxSdfShapes);
    for (int i = 0; i < gp.sdfCount; ++i) {
        const auto& obs = obstacles_[i];
        auto&       dst = gp.sdfShapes[i];
        dst.center[0]      = obs.center.x;
        dst.center[1]      = obs.center.y;
        dst.center[2]      = obs.center.z;
        dst.type           = static_cast<int>(obs.type);  // enum 値 = GPU type コードに一致
        dst.halfExtents[0] = obs.halfExtents.x;
        dst.halfExtents[1] = obs.halfExtents.y;
        dst.halfExtents[2] = obs.halfExtents.z;
        dst._pad           = 0.0f;
    }
    gp.viewProj = viewProj_;

    // GPU 上で全計算 (Density → Force → Integrate) × substeps + PrepareInstances
    compute_.Dispatch(gp, params_.substeps);
}

// ============================================================
//  Draw — Object3d パイプラインで 1 DrawCall 描画
//         Object3DDraw() パス内で呼ぶこと
// ============================================================
void SphSimulator::Draw() {
    renderer_.Draw(compute_.GetInstanceBufferGPUAddr(),
                   static_cast<uint32_t>(compute_.GetParticleCount()));
}

// ============================================================
//  DrawFluid — SSFR 3 パスを実行
//  Draw() の代わりに呼ぶ (両方を同時に呼ばないこと)
// ============================================================
void SphSimulator::DrawFluid(D3D12_CPU_DESCRIPTOR_HANDLE targetRTV,
                              D3D12_CPU_DESCRIPTOR_HANDLE targetDSV) {
    if (!fluidRenderer_.enabled) {
        // SSFR が無効なら通常描画にフォールバック
        Draw();
        return;
    }
    fluidRenderer_.DrawDepthPass(compute_.GetInstancingSrvIndex(),
                                  compute_.GetParticleCount(),
                                  view_, proj_);
    fluidRenderer_.DrawBlurPass();
    fluidRenderer_.DrawShadePass(targetRTV, targetDSV, view_);
}

// ============================================================
//  DrawImGui
// ============================================================
#ifdef USE_IMGUI
// (?) アイコンにマウスを乗せると説明を表示するヘルパー
static void HelpMarker(const char* desc) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
#endif

void SphSimulator::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Begin("SPH シミュレーター (GPU)");
    ImGui::Text("粒子数 : %d  [GPU Compute]", params_.particleCount);

    // ---- プリセット (ワンクリックで質感を切替) ----
    ImGui::Separator();
    ImGui::Text("プリセット");
    if (ImGui::Button("水"))       ApplyPreset(Preset::Water);
    ImGui::SameLine();
    if (ImGui::Button("ハチミツ")) ApplyPreset(Preset::Honey);
    ImGui::SameLine();
    if (ImGui::Button("スライム")) ApplyPreset(Preset::Slime);

    // ---- 再生コントロール ----
    ImGui::Separator();
    ImGui::Text("再生");
    if (paused_) { if (ImGui::Button("再生"))      paused_ = false; }
    else         { if (ImGui::Button("一時停止"))  paused_ = true;  }
    ImGui::SameLine();
    if (ImGui::Button("コマ送り")) { stepOnce_ = true; paused_ = true; }
    ImGui::SameLine();
    if (ImGui::Button("リセット")) Reset();
    ImGui::DragFloat("再生速度", &timeScale_, 0.01f, 0.0f, 2.0f);
    HelpMarker("シミュレーションの速さ。1.0=等速、0.5=スロー、0=停止");

    // ---- シミュレーション ----
    if (ImGui::CollapsingHeader("シミュレーション", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("影響半径",   &params_.smoothingRadius, 0.01f, 0.1f, 5.0f);
        HelpMarker("粒子が影響し合う距離。大きいほど滑らかだが重い。\n注意: 初期値より大きくすると空間ハッシュが近傍を取りこぼす");
        ImGui::DragFloat("静止密度",   &params_.restDensity, 0.1f, 1.0f, 30.0f);
        HelpMarker("流体が保とうとする密度。低いとスカスカ、高いとギュッと詰まる");
        ImGui::DragFloat("圧力剛性",   &params_.stiffness, 1.0f, 1.0f, 500.0f);
        HelpMarker("圧力の強さ。高いほど非圧縮(硬い水)。高すぎると爆発する");
        ImGui::DragFloat("粘性係数",   &params_.viscosity, 0.05f, 0.0f, 50.0f);
        HelpMarker("ねばり。低い=サラサラ(水)、高い=とろとろ(ハチミツ)");
        ImGui::DragFloat("XSPH補正",   &params_.xsphCoeff, 0.01f, 0.0f, 1.0f);
        HelpMarker("粒子の足並みを揃える。高いほど一体感のある流れになる");
        ImGui::DragFloat("表面張力",   &params_.surfaceTension, 0.05f, 0.0f, 20.0f);
        HelpMarker("水面のまとまり。高いほど水滴が丸まり、しずくになりやすい");
        ImGui::DragFloat("粒子質量",   &params_.particleMass, 0.05f, 0.01f, 10.0f);
        ImGui::DragFloat("重力",       &params_.gravity, 0.1f, -30.0f, 0.0f);
        ImGui::DragFloat("壁反発係数", &params_.restitution, 0.01f, 0.0f, 1.0f);
        HelpMarker("壁での跳ね返り。0=跳ねない(液体)、1=完全反射");
        ImGui::SliderInt("サブステップ", &params_.substeps, 1, 5);
        HelpMarker("1フレームの分割数。多いほど安定だが重い");
    }

    // ---- レンダリング ----
    if (ImGui::CollapsingHeader("レンダリング")) {
        ImGui::DragFloat("粒子半径",       &params_.particleRadius, 0.01f, 0.02f, 2.0f);
        ImGui::DragFloat("最大速度（色）", &params_.speedMax, 0.1f, 0.1f, 50.0f);
        HelpMarker("この速度に達すると高速カラーになる");
        ImGui::ColorEdit4("低速カラー",    &params_.colorLow.x);
        ImGui::ColorEdit4("高速カラー",    &params_.colorHigh.x);
    }

    // ---- 境界ボックス ----
    if (ImGui::CollapsingHeader("境界ボックス")) {
        ImGui::DragFloat3("最小座標", &params_.boundMin.x, 0.1f);
        ImGui::DragFloat3("最大座標", &params_.boundMax.x, 0.1f);
        HelpMarker("変更後はリセットで粒子を再配置すると綺麗");

        // 現在の境界サイズがグリッドバッファの上限を超えていたら警告
        float sx = params_.boundMax.x - params_.boundMin.x;
        float sy = params_.boundMax.y - params_.boundMin.y;
        float sz = params_.boundMax.z - params_.boundMin.z;
        if (sx > compute_.GetGridMaxSizeX() ||
            sy > compute_.GetGridMaxSizeY() ||
            sz > compute_.GetGridMaxSizeZ()) {
            ImGui::TextColored({1, 0.3f, 0.3f, 1},
                "! 境界が初期グリッド(%.1f x %.1f x %.1f)を超えています",
                compute_.GetGridMaxSizeX(),
                compute_.GetGridMaxSizeY(),
                compute_.GetGridMaxSizeZ());
            ImGui::TextColored({1, 0.3f, 0.3f, 1},
                "  近傍探索が劣化します。初期サイズを大きく設定してください");
        }
    }

    // ---- 外力 ----
    if (ImGui::CollapsingHeader("外力", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("マウスで操作", &mouseForceEnabled_);
        HelpMarker("ON にして画面を左ドラッグすると、その位置の流体を押せる");
        ImGui::Checkbox("外力 有効",   &params_.extForceActive);
        ImGui::DragFloat3("外力 位置", &params_.extForcePos.x, 0.1f);
        ImGui::DragFloat("外力 半径",  &params_.extForceRadius, 0.1f, 0.1f, 20.0f);
        ImGui::DragFloat("外力 強さ",  &params_.extForceStrength, 1.0f, -300.0f, 300.0f);
        HelpMarker("正=押し出す(噴水)、負=引き寄せる(集める)");
    }

    // ---- スクリーンスペース流体描画 (SSFR) ----
    if (ImGui::CollapsingHeader("流体描画 (SSFR)")) {
        auto& fp = fluidRenderer_.GetParams();
        ImGui::Checkbox("SSFR 有効", &fluidRenderer_.enabled);
        HelpMarker("ON=スムーズな水面描画 / OFF=球メッシュ描画");
        if (fluidRenderer_.enabled) {
            ImGui::DragFloat("ブラー半径",  &fp.blurRadius,   0.1f, 1.0f, 5.0f);
            HelpMarker("粒子の境界を繋ぐブラーの強さ (1-5 テクセル)");
            ImGui::DragFloat("ブラー強度",  &fp.blurFalloff,  0.1f, 0.0f, 5.0f);
            HelpMarker("バイラテラルの深度エッジ保持 (小さいほど均一にブレる)");
            ImGui::DragFloat("法線スケール",&fp.normalScale,  5.0f, 1.0f, 200.0f);
            HelpMarker("深度勾配の増幅率 (大きいほど波紋が鮮明)");
            ImGui::DragFloat("鏡面反射",    &fp.specPower,    4.0f, 4.0f, 256.0f);
            ImGui::DragFloat("フレネル",    &fp.fresnelBias,  0.01f,0.0f, 1.0f);
            HelpMarker("最小不透明度。0=縁が完全透明、1=常に不透明");
            ImGui::ColorEdit4("水色",       &fp.waterColor.x);
            ImGui::DragFloat3("ライト方向", &fp.lightDir.x,   0.1f);
        }
    }

    // ---- SDF 障害物 ----
    if (ImGui::CollapsingHeader("障害物 (SDF)")) {
        // 登録済み障害物の一覧・編集・削除
        for (int i = 0; i < (int)obstacles_.size(); ++i) {
            ImGui::PushID(i);
            auto& obs = obstacles_[i];
            const char* typeName = (obs.type == SdfObstacle::Type::Sphere) ? "球" : "箱";
            const char* dynLabel = obs.dynamic ? " [剛体]" : "";
            bool open = ImGui::TreeNode("obs", "[%d] %s (%s)%s", i, obs.label.c_str(), typeName, dynLabel);
            if (open) {
                ImGui::DragFloat3("中心", &obs.center.x, 0.1f);
                if (obs.type == SdfObstacle::Type::Sphere) {
                    ImGui::DragFloat("半径", &obs.halfExtents.x, 0.1f, 0.1f, 20.0f);
                } else {
                    ImGui::DragFloat3("半辺長", &obs.halfExtents.x, 0.1f, 0.1f, 20.0f);
                }
                ImGui::Separator();
                ImGui::Checkbox("物理剛体", &obs.dynamic);
                HelpMarker("ON にすると重力・浮力・抵抗で動く。質量 < 流体密度×体積 なら浮く");
                if (obs.dynamic) {
                    ImGui::DragFloat("質量", &obs.mass, 0.1f, 0.01f, 200.0f);
                    ImGui::Text("速度: (%.2f, %.2f, %.2f)", obs.velocity.x, obs.velocity.y, obs.velocity.z);
                    if (ImGui::Button("静止")) obs.velocity = {};
                }
                if (ImGui::Button("削除")) {
                    obstacles_.erase(obstacles_.begin() + i);
                    ImGui::TreePop();
                    ImGui::PopID();
                    break;
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        // ---- 追加フォーム: 障害物 ----
        ImGui::Separator();
        ImGui::TextColored({1,0.6f,0,1}, "障害物 (流体を押しのける)");
        ImGui::DragFloat3("中心##sph", ui_.sphCenter, 0.1f);
        ImGui::DragFloat("半径##sph",  &ui_.sphRadius, 0.1f, 0.1f, 20.0f);
        if (ImGui::Button("球を追加") && (int)obstacles_.size() < kMaxSdfShapes)
            AddSphere({ui_.sphCenter[0], ui_.sphCenter[1], ui_.sphCenter[2]}, ui_.sphRadius);

        ImGui::Spacing();
        ImGui::DragFloat3("中心##box",   ui_.boxCenter, 0.1f);
        ImGui::DragFloat3("半辺長##box", ui_.boxHalf,   0.1f, 0.1f, 20.0f);
        if (ImGui::Button("箱を追加") && (int)obstacles_.size() < kMaxSdfShapes)
            AddBox({ui_.boxCenter[0], ui_.boxCenter[1], ui_.boxCenter[2]},
                   {ui_.boxHalf[0],   ui_.boxHalf[1],   ui_.boxHalf[2]});

        // ---- 追加フォーム: コンテナ ----
        ImGui::Separator();
        ImGui::TextColored({0,0.8f,0.8f,1}, "コンテナ (水槽 — この形の中に流体が入る)");
        HelpMarker("コンテナを追加すると AABB が自動で合わせられます。追加後にリセットを推奨");

        ImGui::DragFloat3("中心##csph",  ui_.cntSphCenter, 0.1f);
        ImGui::DragFloat("半径##csph",   &ui_.cntSphRadius, 0.1f, 1.0f, 30.0f);
        if (ImGui::Button("球コンテナを追加") && (int)obstacles_.size() < kMaxSdfShapes)
            AddContainerSphere({ui_.cntSphCenter[0], ui_.cntSphCenter[1], ui_.cntSphCenter[2]},
                               ui_.cntSphRadius);

        ImGui::Spacing();
        ImGui::DragFloat3("中心##cbox",   ui_.cntBoxCenter, 0.1f);
        ImGui::DragFloat3("半辺長##cbox", ui_.cntBoxHalf,   0.1f, 1.0f, 30.0f);
        if (ImGui::Button("箱コンテナを追加") && (int)obstacles_.size() < kMaxSdfShapes)
            AddContainerBox({ui_.cntBoxCenter[0], ui_.cntBoxCenter[1], ui_.cntBoxCenter[2]},
                            {ui_.cntBoxHalf[0],   ui_.cntBoxHalf[1],   ui_.cntBoxHalf[2]});

        ImGui::Spacing();
        ImGui::DragFloat3("中心##ccyl",  ui_.cntCylCenter, 0.1f);
        ImGui::DragFloat("半径##ccyl",   &ui_.cntCylRadius, 0.1f, 1.0f, 30.0f);
        ImGui::DragFloat("半高さ##ccyl", &ui_.cntCylHalfH,  0.1f, 1.0f, 30.0f);
        if (ImGui::Button("円柱コンテナを追加") && (int)obstacles_.size() < kMaxSdfShapes)
            AddContainerCylinder({ui_.cntCylCenter[0], ui_.cntCylCenter[1], ui_.cntCylCenter[2]},
                                 ui_.cntCylRadius, ui_.cntCylHalfH);

        if (!obstacles_.empty()) {
            ImGui::Separator();
            if (ImGui::Button("全削除")) obstacles_.clear();
        }

        ImGui::TextDisabled("最大 %d 個", kMaxSdfShapes);
    }

    ImGui::End();
#endif
}

// ============================================================
//  DrawBounds — バウンディングボックスの 12 辺をラインで描画
// ============================================================
void SphSimulator::DrawBounds(const TuboEngine::Math::Vector4& color) {
    auto* lm = TuboEngine::LineManager::GetInstance();
    using V3 = TuboEngine::Math::Vector3;
    const V3& mn = params_.boundMin;
    const V3& mx = params_.boundMax;

    // 底面 (Y = min)
    lm->DrawLine({mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, color);
    lm->DrawLine({mx.x, mn.y, mn.z}, {mx.x, mn.y, mx.z}, color);
    lm->DrawLine({mx.x, mn.y, mx.z}, {mn.x, mn.y, mx.z}, color);
    lm->DrawLine({mn.x, mn.y, mx.z}, {mn.x, mn.y, mn.z}, color);

    // 上面 (Y = max)
    lm->DrawLine({mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z}, color);
    lm->DrawLine({mx.x, mx.y, mn.z}, {mx.x, mx.y, mx.z}, color);
    lm->DrawLine({mx.x, mx.y, mx.z}, {mn.x, mx.y, mx.z}, color);
    lm->DrawLine({mn.x, mx.y, mx.z}, {mn.x, mx.y, mn.z}, color);

    // 縦辺 x4
    lm->DrawLine({mn.x, mn.y, mn.z}, {mn.x, mx.y, mn.z}, color);
    lm->DrawLine({mx.x, mn.y, mn.z}, {mx.x, mx.y, mn.z}, color);
    lm->DrawLine({mx.x, mn.y, mx.z}, {mx.x, mx.y, mx.z}, color);
    lm->DrawLine({mn.x, mn.y, mx.z}, {mn.x, mx.y, mx.z}, color);

    // 外力点の可視化 (有効時、力点に影響半径サイズの十字を描画)
    if (params_.extForceActive) {
        const V3&   c  = params_.extForcePos;
        const float r  = params_.extForceRadius;
        const TuboEngine::Math::Vector4 fc = {1.0f, 0.4f, 0.1f, 1.0f};
        lm->DrawLine({c.x - r, c.y, c.z}, {c.x + r, c.y, c.z}, fc);
        lm->DrawLine({c.x, c.y - r, c.z}, {c.x, c.y + r, c.z}, fc);
        lm->DrawLine({c.x, c.y, c.z - r}, {c.x, c.y, c.z + r}, fc);
    }

    // SDF 障害物 / コンテナの可視化
    // 障害物 = オレンジ、コンテナ = シアン
    for (const auto& obs : obstacles_) {
        const TuboEngine::Math::Vector4 oc = obs.IsContainer()
            ? TuboEngine::Math::Vector4{0.0f, 0.9f, 0.9f, 1.0f}  // シアン
            : TuboEngine::Math::Vector4{1.0f, 0.5f, 0.0f, 1.0f}; // オレンジ

        if (obs.type == SdfObstacle::Type::Box ||
            obs.type == SdfObstacle::Type::BoxContainer) {
            V3 omn = { obs.center.x - obs.halfExtents.x,
                       obs.center.y - obs.halfExtents.y,
                       obs.center.z - obs.halfExtents.z };
            V3 omx = { obs.center.x + obs.halfExtents.x,
                       obs.center.y + obs.halfExtents.y,
                       obs.center.z + obs.halfExtents.z };
            lm->DrawLine({omn.x, omn.y, omn.z}, {omx.x, omn.y, omn.z}, oc);
            lm->DrawLine({omx.x, omn.y, omn.z}, {omx.x, omn.y, omx.z}, oc);
            lm->DrawLine({omx.x, omn.y, omx.z}, {omn.x, omn.y, omx.z}, oc);
            lm->DrawLine({omn.x, omn.y, omx.z}, {omn.x, omn.y, omn.z}, oc);
            lm->DrawLine({omn.x, omx.y, omn.z}, {omx.x, omx.y, omn.z}, oc);
            lm->DrawLine({omx.x, omx.y, omn.z}, {omx.x, omx.y, omx.z}, oc);
            lm->DrawLine({omx.x, omx.y, omx.z}, {omn.x, omx.y, omx.z}, oc);
            lm->DrawLine({omn.x, omx.y, omx.z}, {omn.x, omx.y, omn.z}, oc);
            lm->DrawLine({omn.x, omn.y, omn.z}, {omn.x, omx.y, omn.z}, oc);
            lm->DrawLine({omx.x, omn.y, omn.z}, {omx.x, omx.y, omn.z}, oc);
            lm->DrawLine({omx.x, omn.y, omx.z}, {omx.x, omx.y, omx.z}, oc);
            lm->DrawLine({omn.x, omn.y, omx.z}, {omn.x, omx.y, omx.z}, oc);
        } else if (obs.type == SdfObstacle::Type::CylinderContainer) {
            // 円柱: 上下2円を8点近似 + 縦辺4本
            const V3& c = obs.center;
            const float r = obs.halfExtents.x;
            const float h = obs.halfExtents.y;
            const int segs = 8;
            for (int k = 0; k < segs; ++k) {
                float a0 = kPi * 2.0f * k / segs;
                float a1 = kPi * 2.0f * (k + 1) / segs;
                // 上面
                lm->DrawLine({c.x + r*std::cos(a0), c.y + h, c.z + r*std::sin(a0)},
                             {c.x + r*std::cos(a1), c.y + h, c.z + r*std::sin(a1)}, oc);
                // 下面
                lm->DrawLine({c.x + r*std::cos(a0), c.y - h, c.z + r*std::sin(a0)},
                             {c.x + r*std::cos(a1), c.y - h, c.z + r*std::sin(a1)}, oc);
                // 縦辺 (4本)
                if (k % 2 == 0)
                    lm->DrawLine({c.x + r*std::cos(a0), c.y - h, c.z + r*std::sin(a0)},
                                 {c.x + r*std::cos(a0), c.y + h, c.z + r*std::sin(a0)}, oc);
            }
        } else {
            // 球 / 球コンテナ: 3 軸の十字線
            const V3&   c = obs.center;
            const float r = obs.halfExtents.x;
            lm->DrawLine({c.x - r, c.y, c.z}, {c.x + r, c.y, c.z}, oc);
            lm->DrawLine({c.x, c.y - r, c.z}, {c.x, c.y + r, c.z}, oc);
            lm->DrawLine({c.x, c.y, c.z - r}, {c.x, c.y, c.z + r}, oc);
        }
    }
}

// ============================================================
//  GenerateInitialParticles — CPU でグリッド配置を生成
// ============================================================
std::vector<SphParticle> SphSimulator::GenerateInitialParticles() const {
    std::vector<SphParticle> particles;
    particles.reserve(params_.particleCount);

    const float spacing    = params_.smoothingRadius * 0.55f;
    const int   dim        = static_cast<int>(std::cbrt(float(params_.particleCount))) + 1;
    const float gridExtent = (dim - 1) * spacing;

    // グリッドをボックス中央(XZ)・底面(Y)に配置
    // ボックス外に粒子が出ないよう中央揃え
    const TuboEngine::Math::Vector3 start = {
        (params_.boundMin.x + params_.boundMax.x) * 0.5f - gridExtent * 0.5f,
        params_.boundMin.y + spacing,   // 底面から積み上げる (重力で落下するので自然)
        (params_.boundMin.z + params_.boundMax.z) * 0.5f - gridExtent * 0.5f
    };

    int added = 0;
    for (int iz = 0; iz < dim && added < params_.particleCount; ++iz)
    for (int iy = 0; iy < dim && added < params_.particleCount; ++iy)
    for (int ix = 0; ix < dim && added < params_.particleCount; ++ix) {
        SphParticle p{};
        p.position = { start.x + ix * spacing,
                       start.y + iy * spacing,
                       start.z + iz * spacing };
        p.density  = params_.restDensity;
        particles.push_back(p);
        ++added;
    }
    return particles;
}

// ============================================================
//  ApplyPreset — 質感プリセットを適用 (粒子数/境界は変えない)
// ============================================================
void SphSimulator::ApplyPreset(Preset preset) {
    switch (preset) {
    case Preset::Water:   // サラサラの水
        params_.viscosity      = 1.0f;
        params_.stiffness      = 200.0f;
        params_.xsphCoeff      = 0.15f;
        params_.restitution    = 0.02f;
        params_.surfaceTension = 2.0f;
        break;
    case Preset::Honey:   // とろとろのハチミツ
        params_.viscosity      = 40.0f;
        params_.stiffness      = 150.0f;
        params_.xsphCoeff      = 0.30f;
        params_.restitution    = 0.0f;
        params_.surfaceTension = 1.0f;
        break;
    case Preset::Slime:   // ぷるぷるのスライム (高めの表面張力で丸まる)
        params_.viscosity      = 15.0f;
        params_.stiffness      = 250.0f;
        params_.xsphCoeff      = 0.45f;
        params_.restitution    = 0.0f;
        params_.surfaceTension = 8.0f;
        break;
    }
}

// ============================================================
//  UpdateMouseForce — マウス左ドラッグで外力点を操作
//  スクリーン座標をボックス中心の深度平面へアンプロジェクトする
// ============================================================
void SphSimulator::UpdateMouseForce() {
    if (!mouseForceEnabled_) {
        if (mouseDriving_) { params_.extForceActive = false; mouseDriving_ = false; }
        return;
    }

    auto* input = TuboEngine::Input::GetInstance();

#ifdef USE_IMGUI
    // ImGui ウィンドウ操作中はマウス外力を無効化
    if (ImGui::GetIO().WantCaptureMouse) {
        if (mouseDriving_) { params_.extForceActive = false; mouseDriving_ = false; }
        return;
    }
#endif

    if (input->IsPressMouse(0)) {
        const float W = static_cast<float>(TuboEngine::WinApp::GetInstance()->GetClientWidth());
        const float H = static_cast<float>(TuboEngine::WinApp::GetInstance()->GetClientHeight());
        const auto  mp = input->GetMousePosition();

        // スクリーン → NDC
        const float ndcX = 2.0f * mp.x / W - 1.0f;
        const float ndcY = 1.0f - 2.0f * mp.y / H;

        // ボックス中心の深度 (NDC z) を基準面に使う
        const TuboEngine::Math::Vector3 center = {
            (params_.boundMin.x + params_.boundMax.x) * 0.5f,
            (params_.boundMin.y + params_.boundMax.y) * 0.5f,
            (params_.boundMin.z + params_.boundMax.z) * 0.5f
        };
        const TuboEngine::Math::Vector4 cClip = MulRowVec4({center.x, center.y, center.z, 1.0f}, viewProj_);
        const float ndcZ = (std::abs(cClip.w) > 1e-6f) ? cClip.z / cClip.w : 0.5f;

        // NDC → ワールド (逆 ViewProjection)
        const TuboEngine::Math::Matrix4x4 invVP = TuboEngine::Math::Inverse(viewProj_);
        const TuboEngine::Math::Vector4 wp = MulRowVec4({ndcX, ndcY, ndcZ, 1.0f}, invVP);
        if (std::abs(wp.w) > 1e-6f) {
            params_.extForcePos    = { wp.x / wp.w, wp.y / wp.w, wp.z / wp.w };
            params_.extForceActive = true;
            mouseDriving_          = true;
        }
    } else if (mouseDriving_) {
        // ボタンを離したらマウス駆動の外力をOFF
        params_.extForceActive = false;
        mouseDriving_          = false;
    }
}

// ============================================================
//  AddSphere / AddBox / ClearObstacles — SDF 障害物管理
// ============================================================
void SphSimulator::AddSphere(const TuboEngine::Math::Vector3& center, float radius,
                              const std::string& label) {
    if ((int)obstacles_.size() >= kMaxSdfShapes) return;
    SdfObstacle obs;
    obs.type        = SdfObstacle::Type::Sphere;
    obs.center      = center;
    obs.halfExtents = { radius, radius, radius };
    obs.label       = label.empty() ? "Sphere" : label;
    obstacles_.push_back(obs);
}

void SphSimulator::AddBox(const TuboEngine::Math::Vector3& center,
                           const TuboEngine::Math::Vector3& halfExtents,
                           const std::string& label) {
    if ((int)obstacles_.size() >= kMaxSdfShapes) return;
    SdfObstacle obs;
    obs.type        = SdfObstacle::Type::Box;
    obs.center      = center;
    obs.halfExtents = halfExtents;
    obs.label       = label.empty() ? "Box" : label;
    obstacles_.push_back(obs);
}

void SphSimulator::ClearObstacles() {
    obstacles_.clear();
}

// ---- コンテナ追加: AABB を自動でコンテナに合わせる ----
// 追加後は Reset() を呼ぶと粒子がコンテナ内に再配置される

void SphSimulator::AddContainerSphere(const TuboEngine::Math::Vector3& center,
                                       float radius, const std::string& label) {
    if ((int)obstacles_.size() >= kMaxSdfShapes) return;
    SdfObstacle obs;
    obs.type        = SdfObstacle::Type::SphereContainer;
    obs.center      = center;
    obs.halfExtents = { radius, radius, radius };
    obs.label       = label.empty() ? "SphereContainer" : label;
    obstacles_.push_back(obs);
    // AABB をコンテナに合わせる
    params_.boundMin = { center.x - radius, center.y - radius, center.z - radius };
    params_.boundMax = { center.x + radius, center.y + radius, center.z + radius };
}

void SphSimulator::AddContainerBox(const TuboEngine::Math::Vector3& center,
                                    const TuboEngine::Math::Vector3& halfExtents,
                                    const std::string& label) {
    if ((int)obstacles_.size() >= kMaxSdfShapes) return;
    SdfObstacle obs;
    obs.type        = SdfObstacle::Type::BoxContainer;
    obs.center      = center;
    obs.halfExtents = halfExtents;
    obs.label       = label.empty() ? "BoxContainer" : label;
    obstacles_.push_back(obs);
    params_.boundMin = { center.x - halfExtents.x, center.y - halfExtents.y, center.z - halfExtents.z };
    params_.boundMax = { center.x + halfExtents.x, center.y + halfExtents.y, center.z + halfExtents.z };
}

void SphSimulator::AddContainerCylinder(const TuboEngine::Math::Vector3& center,
                                         float radius, float halfHeight,
                                         const std::string& label) {
    if ((int)obstacles_.size() >= kMaxSdfShapes) return;
    SdfObstacle obs;
    obs.type        = SdfObstacle::Type::CylinderContainer;
    obs.center      = center;
    obs.halfExtents = { radius, halfHeight, radius };
    obs.label       = label.empty() ? "CylinderContainer" : label;
    obstacles_.push_back(obs);
    params_.boundMin = { center.x - radius, center.y - halfHeight, center.z - radius };
    params_.boundMax = { center.x + radius, center.y + halfHeight, center.z + radius };
}

void SphSimulator::AddDynamicSphere(const TuboEngine::Math::Vector3& center, float radius,
                                     float mass, const std::string& label) {
    AddSphere(center, radius, label);
    obstacles_.back().dynamic = true;
    obstacles_.back().mass    = mass;
}

void SphSimulator::AddDynamicBox(const TuboEngine::Math::Vector3& center,
                                  const TuboEngine::Math::Vector3& halfExtents,
                                  float mass, const std::string& label) {
    AddBox(center, halfExtents, label);
    obstacles_.back().dynamic = true;
    obstacles_.back().mass    = mass;
}

// ============================================================
//  IntegrateRigidBodies — 浮力・粘性抵抗・重力で剛体を積分
//
//  流体 → 剛体 の力:
//    F_buoy = ρ_fluid × V_submerged × |g|  (アルキメデスの原理)
//    F_drag = -k × v  (速度に比例する粘性抵抗)
//  剛体 → 流体 の力:
//    CsIntegrate の SDF 押し出しがすでに担当 (Phase 1)
//    剛体が動けば次フレームの SDF 位置が変わり、流体が押し出される
// ============================================================
void SphSimulator::IntegrateRigidBodies(float dt) {
    if (dt <= 0.0f) return;

    using V3 = TuboEngine::Math::Vector3;
    const float g    = std::abs(params_.gravity);     // 重力加速度 (正値)
    const float rho0 = params_.restDensity;           // 流体の静止密度

    // 流体表面の高さを推定 (粒子総体積 ÷ 水槽断面積)
    const float fluidVolume = float(params_.particleCount) * params_.particleMass / rho0;
    const float tankArea    = (params_.boundMax.x - params_.boundMin.x)
                            * (params_.boundMax.z - params_.boundMin.z);
    const float hFluid      = params_.boundMin.y + fluidVolume / (tankArea + 1e-6f);

    for (auto& obs : obstacles_) {
        if (!obs.dynamic) continue;

        // ---- 浮力: 水没体積 × ρ_fluid × g (上向き) ----
        float vSub = 0.0f;
        if (obs.type == SdfObstacle::Type::Sphere) {
            const float R = obs.halfExtents.x;
            // 球の下端から流体面までの浸水深さ (0 〜 2R)
            const float hSub = std::max(0.0f,
                               std::min(2.0f * R, hFluid - (obs.center.y - R)));
            // 球面帽の体積: π h² (3R - h) / 3
            vSub = kPi * hSub * hSub * (3.0f * R - hSub) / 3.0f;
        } else {
            // 箱: 浸水高さ × 底面積
            const float yBot = obs.center.y - obs.halfExtents.y;
            const float yTop = obs.center.y + obs.halfExtents.y;
            const float hSub = std::max(0.0f, std::min(yTop, hFluid) - yBot);
            vSub = hSub * (2.0f * obs.halfExtents.x) * (2.0f * obs.halfExtents.z);
        }
        const float fBuoy = rho0 * vSub * g;  // 上向き

        // ---- 粘性抵抗: -k × v (流体の粘性に比例) ----
        // 代表断面積を形状から推定してストークス則に近い係数を得る
        float crossSection;
        if (obs.type == SdfObstacle::Type::Sphere) {
            crossSection = kPi * obs.halfExtents.x * obs.halfExtents.x;
        } else {
            // 最大断面積 (各軸ペア)
            crossSection = std::max({obs.halfExtents.x * obs.halfExtents.y,
                                     obs.halfExtents.y * obs.halfExtents.z,
                                     obs.halfExtents.z * obs.halfExtents.x}) * 4.0f;
        }
        const float kDrag = params_.viscosity * 0.04f * crossSection;

        // ---- 合力・積分 ----
        const float ax = (-kDrag * obs.velocity.x)                 / obs.mass;
        const float ay = (-obs.mass * g + fBuoy - kDrag * obs.velocity.y) / obs.mass;
        const float az = (-kDrag * obs.velocity.z)                 / obs.mass;

        obs.velocity.x += ax * dt;
        obs.velocity.y += ay * dt;
        obs.velocity.z += az * dt;
        obs.center.x   += obs.velocity.x * dt;
        obs.center.y   += obs.velocity.y * dt;
        obs.center.z   += obs.velocity.z * dt;

        // ---- AABB 壁との衝突反射 ----
        const V3 he = (obs.type == SdfObstacle::Type::Sphere)
                      ? V3{obs.halfExtents.x, obs.halfExtents.x, obs.halfExtents.x}
                      : obs.halfExtents;
        const float rest = params_.restitution;

        if (obs.center.x - he.x < params_.boundMin.x) {
            obs.center.x = params_.boundMin.x + he.x;
            obs.velocity.x =  std::abs(obs.velocity.x) * rest;
        }
        if (obs.center.x + he.x > params_.boundMax.x) {
            obs.center.x = params_.boundMax.x - he.x;
            obs.velocity.x = -std::abs(obs.velocity.x) * rest;
        }
        if (obs.center.y - he.y < params_.boundMin.y) {
            obs.center.y = params_.boundMin.y + he.y;
            obs.velocity.y =  std::abs(obs.velocity.y) * rest;
        }
        if (obs.center.y + he.y > params_.boundMax.y) {
            obs.center.y = params_.boundMax.y - he.y;
            obs.velocity.y = -std::abs(obs.velocity.y) * rest;
        }
        if (obs.center.z - he.z < params_.boundMin.z) {
            obs.center.z = params_.boundMin.z + he.z;
            obs.velocity.z =  std::abs(obs.velocity.z) * rest;
        }
        if (obs.center.z + he.z > params_.boundMax.z) {
            obs.center.z = params_.boundMax.z - he.z;
            obs.velocity.z = -std::abs(obs.velocity.z) * rest;
        }
    }
}

