#include "ParticleManager.h"
#include "ImGuiManager.h"
#include "Camera.h"
#include "ParticlePresetJson.h" // ParticlePreset ⇔ JSON の対応表（項目はここで1回だけ定義）
#include "JsonManager.h"
#include <filesystem>
#include <algorithm>
#include <cstdarg>
#include "Effects/Primitive/PrimitiveEmitter.h"
#include "Effects/Ring/RingEmitter.h"
#include "Effects/Cylinder/CylinderEmitter.h"
#include "Effects/Original/OriginalEmitter.h"
#include "Effects/OrbitTrail/OrbitTrailEmitter.h"
#include "Effects/Default/DefaultEmitter.h"
#include "Effects/Aura/AuraEmitter.h"


static void ApplyParticleManagerTheme(int themeId = 0) {
#ifdef USE_IMGUI
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding  = 4.0f;
    style.GrabRounding   = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing  = ImVec2(8, 6);
    style.WindowPadding= ImVec2(10, 10);
    style.TabRounding  = 4.0f;

    ImVec4* colors = style.Colors;
    if (themeId == 0) { // Dark
        colors[ImGuiCol_WindowBg]        = ImVec4(0.12f,0.12f,0.14f,1.0f);
        colors[ImGuiCol_ChildBg]         = ImVec4(0.10f,0.10f,0.12f,1.0f);
        colors[ImGuiCol_FrameBg]         = ImVec4(0.20f,0.22f,0.26f,1.0f);
        colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.30f,0.34f,0.40f,1.0f);
        colors[ImGuiCol_FrameBgActive]   = ImVec4(0.36f,0.40f,0.48f,1.0f);
        colors[ImGuiCol_TitleBg]         = ImVec4(0.08f,0.08f,0.10f,1.0f);
        colors[ImGuiCol_TitleBgActive]   = ImVec4(0.16f,0.18f,0.22f,1.0f);
        colors[ImGuiCol_Button]          = ImVec4(0.25f,0.28f,0.34f,1.0f);
        colors[ImGuiCol_ButtonHovered]   = ImVec4(0.35f,0.40f,0.48f,1.0f);
        colors[ImGuiCol_ButtonActive]    = ImVec4(0.40f,0.46f,0.55f,1.0f);
        colors[ImGuiCol_Header]          = ImVec4(0.22f,0.26f,0.32f,1.0f);
        colors[ImGuiCol_HeaderHovered]   = ImVec4(0.32f,0.38f,0.46f,1.0f);
        colors[ImGuiCol_HeaderActive]    = ImVec4(0.38f,0.44f,0.52f,1.0f);
        colors[ImGuiCol_Tab]             = ImVec4(0.18f,0.20f,0.26f,1.0f);
        colors[ImGuiCol_TabHovered]      = ImVec4(0.34f,0.38f,0.46f,1.0f);
        colors[ImGuiCol_TabActive]       = ImVec4(0.28f,0.32f,0.40f,1.0f);
        colors[ImGuiCol_PlotHistogram]   = ImVec4(0.26f,0.70f,0.55f,1.0f);
        colors[ImGuiCol_SliderGrab]      = ImVec4(0.42f,0.60f,1.00f,1.0f);
        colors[ImGuiCol_SliderGrabActive]= ImVec4(0.58f,0.74f,1.00f,1.0f);
    } else if (themeId == 1) { // Light
        ImGui::StyleColorsLight();
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.20f,0.55f,0.35f,1.0f);
    } else { // HighContrast
        ImGui::StyleColorsDark();
        colors[ImGuiCol_WindowBg]      = ImVec4(0.05f,0.05f,0.05f,1.0f);
        colors[ImGuiCol_Button]        = ImVec4(0.30f,0.05f,0.05f,1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.55f,0.15f,0.15f,1.0f);
        colors[ImGuiCol_ButtonActive]  = ImVec4(0.70f,0.20f,0.20f,1.0f);
        colors[ImGuiCol_Header]        = ImVec4(0.20f,0.25f,0.55f,1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f,0.35f,0.75f,1.0f);
        colors[ImGuiCol_HeaderActive]  = ImVec4(0.40f,0.45f,0.85f,1.0f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.88f,0.70f,0.10f,1.0f);
    }
#endif // USE_IMGUI
}

namespace {
    inline void FixMinMax(float& mn, float& mx) { if (mn > mx) std::swap(mn, mx); }
    inline void FixMinMax(TuboEngine::Math::Vector3& mn, TuboEngine::Math::Vector3& mx) {
	    FixMinMax(mn.x, mx.x);
	    FixMinMax(mn.y, mx.y);
	    FixMinMax(mn.z, mx.z);
    }
    inline void FixMinMax(Vector4& mn, Vector4& mx) { FixMinMax(mn.x, mx.x); FixMinMax(mn.y, mx.y); FixMinMax(mn.z, mx.z); FixMinMax(mn.w, mx.w); }
    inline void FixPresetRanges(ParticlePreset& p) {
        FixMinMax(p.lifeMin, p.lifeMax);
        FixMinMax(p.posMin, p.posMax);
        FixMinMax(p.velMin, p.velMax);
        FixMinMax(p.scaleMin, p.scaleMax);
        FixMinMax(p.colMin, p.colMax);
    }
}

namespace {
    using Json = TuboEngine::JsonManager::Json;

    // エミッター1個を JSON にする（プリセット＋種類＋種類固有の値）
    Json EmitterToJson(const IParticleEmitter& emitter, const std::string& type) {
        Json j = emitter.GetPreset(); // 項目は ParticlePresetJson.h の一覧どおり
        j["type"] = type;
        if (auto* cyl = dynamic_cast<const CylinderEmitter*>(&emitter)) {
            j["spinSpeedY"] = cyl->GetSpinSpeedY();
        }
        return j;
    }

    // JSON 1個からエミッターを作る。作れたら true
    bool CreateEmitterFromJson(TuboEngine::ParticleManager& manager, const Json& j) {
        if (!j.is_object()) {
            return false;
        }
        ParticlePreset preset; // 無い項目は ParticlePreset の初期値
        try {
            j.get_to(preset);
        } catch (const std::exception&) {
            return false;
        }
        IParticleEmitter* emitter = manager.CreateEmitterByType(j.value("type", std::string("Primitive")), preset);
        if (auto* cyl = dynamic_cast<CylinderEmitter*>(emitter)) {
            float spin = cyl->GetSpinSpeedY();
            TuboEngine::JsonManager::Read(j, "spinSpeedY", spin);
            cyl->SetSpinSpeedY(spin);
        }
        return emitter != nullptr;
    }

    // 保存ファイルの形式の違いを吸収して "emitters" 配列を取り出す
    //   version 2: { "version": 2, "snapshot": { "emitters": [...] } }
    //   旧形式   : { "emitters": [...] }
    const Json* FindEmittersArray(const Json& root) {
        const Json* container = &root;
        if (root.is_object() && root.contains("snapshot")) {
            container = &root["snapshot"];
        }
        if (!container->is_object() || !container->contains("emitters") || !(*container)["emitters"].is_array()) {
            return nullptr;
        }
        return &(*container)["emitters"];
    }
}

TuboEngine::ParticleManager::ParticleManager() {
    RegisterDefaultEmitters();
    // 初期ロード (存在すれば)
    InitialLoad("Resources/Particles/all.json");
}

void TuboEngine::ParticleManager::RegisterDefaultEmitters() {
    emitterRegistry_.clear();
    emitterRegistry_["Default"]   = [](){ return std::make_unique<DefaultEmitter>(); };
    emitterRegistry_["Primitive"] = [](){ return std::make_unique<PrimitiveEmitter>(); };
    emitterRegistry_["Ring"]      = [](){ return std::make_unique<RingEmitter>(); };
    emitterRegistry_["Cylinder"]  = [](){ return std::make_unique<CylinderEmitter>(); };
    emitterRegistry_["Original"]  = [](){ return std::make_unique<OriginalEmitter>(); };
    emitterRegistry_["OrbitTrail"] = [](){ return std::make_unique<OrbitTrailEmitter>(); };
    emitterRegistry_["Aura"]          = [](){ return std::make_unique<AuraEmitter>(); };
}

std::string TuboEngine::ParticleManager::DetectEmitterType(IParticleEmitter* e) const {
    if (dynamic_cast<DefaultEmitter*>(e)) return "Default";
    if (dynamic_cast<RingEmitter*>(e)) return "Ring";
    if (dynamic_cast<CylinderEmitter*>(e)) return "Cylinder";
    if (dynamic_cast<OriginalEmitter*>(e)) return "Original";
    if (dynamic_cast<OrbitTrailEmitter*>(e)) return "OrbitTrail";
    // Aura は PrimitiveEmitter 派生のため Primitive 判定より先に調べる
    if (dynamic_cast<AuraEmitter*>(e)) return "Aura";
    return "Primitive";
}

IParticleEmitter* TuboEngine::ParticleManager::CreateEmitterByType(const std::string& typeName, const ParticlePreset& preset) {
    auto it = emitterRegistry_.find(typeName);
    std::unique_ptr<IParticleEmitter> ptr;
    if (it != emitterRegistry_.end()) ptr = it->second();
    else if (auto it2 = emitterRegistry_.find("Primitive"); it2 != emitterRegistry_.end()) ptr = it2->second();
    if (!ptr) return nullptr;
    ParticlePreset adjusted = preset;
    adjusted.name = GenerateUniqueName(adjusted.name.empty() ? "Emitter" : adjusted.name);
    ptr->Initialize(adjusted);
    IParticleEmitter* raw = ptr.get();
    emitters_.push_back(std::move(ptr));
    SetStatus("Created '%s' (%s)", adjusted.name.c_str(), typeName.c_str());
    MarkChanged();
    return raw;
}

void TuboEngine::ParticleManager::SetStatus(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    vsnprintf(statusMsg_, sizeof(statusMsg_), fmt, args);
    va_end(args); statusTimer_ = 3.0f;
}

void TuboEngine::ParticleManager::MarkChanged() { if (!applyingSnapshot_) changedThisFrame_ = true; }

std::string TuboEngine::ParticleManager::GenerateUniqueName(const std::string& base) const {
    if (base.empty()) return GenerateUniqueName("Emitter");
    bool exists = false; for (auto& e : emitters_) if (e->GetName() == base) { exists = true; break; }
    if (!exists) return base; int counter = 2; while (true) { std::string c = base + "(" + std::to_string(counter) + ")"; bool dup=false; for (auto& e:emitters_) if (e->GetName()==c){dup=true;break;} if(!dup) return c; ++counter; }
}

void TuboEngine::ParticleManager::Update(float dt, TuboEngine::Camera* cam) {
    for (auto& e : emitters_) {
        auto& preset = e->GetPreset();
        if (preset.autoEmit && preset.emitRate > 0.0f) {
            float interval = 1.0f / preset.emitRate;
            preset._emitAccum += dt;
            while (preset._emitAccum >= interval) { e->Emit(1); preset._emitAccum -= interval; }
        }
        e->Update(dt, cam);
    }
    if (statusTimer_ > 0.0f) { statusTimer_ -= dt; if (statusTimer_ <= 0.0f) statusMsg_[0] = '\0'; }
    if (changedThisFrame_) { CaptureHistory(); changedThisFrame_ = false; }
    UpdatePreview(dt, cam);
    if (previewPendingDestroy_) { previewEmitter_.reset(); previewType_ = -1; previewPendingDestroy_ = false; }
}

void TuboEngine::ParticleManager::Draw() {
    // 描画コマンドを積む前に、保留中の Undo/Redo 復元を適用する。
    // （フレーム途中で emitters を作り直すと、コマンドリストが参照中のGPUリソースを
    //   破棄してしまい OBJECT_DELETED_WHILE_STILL_IN_USE で落ちるのを防ぐ）
    if (hasPendingApply_) { ApplySnapshot(pendingApplyJson_); hasPendingApply_ = false; }
    auto* cmd = TuboEngine::DirectXCommon::GetInstance()->GetCommandList().Get();
    for (auto& e : emitters_) e->Draw(cmd);
    DrawPreview(cmd);
}

void TuboEngine::ParticleManager::DrawStatusBar() {
#ifdef USE_IMGUI
    if (statusMsg_[0] != '\0') ImGui::TextColored(ImVec4(0.2f,0.6f,0.9f,1.0f), "%s", statusMsg_);
#endif
}

void TuboEngine::ParticleManager::DrawTemplatesSection() {
#ifdef USE_IMGUI
    ImGui::Separator(); ImGui::Text("Templates"); static int tmplIndex = 0;
    const char* items = "Default\0Smoke\0RingBurst\0Fountain\0Radial\0OrbitTrail\0Aura\0HitEffect\0Cylinder\0";
    ImGui::Combo("Template", &tmplIndex, items);
    if (ImGui::Button("Add Template")) {
        ParticlePreset p{}; switch (tmplIndex) {
        case 0: // Default (no gravity/drag)
            p.name = "Default"; p.texture = "particle.png"; p.emitRate = 30.0f; p.autoEmit = true;
            p.lifeMin = 0.6f; p.lifeMax = 1.2f; p.scaleStart = {0.3f,0.3f,0.3f}; p.scaleEnd = {0.1f,0.1f,0.1f};
            p.colorStart = {1,1,1,0.9f}; p.colorEnd = {1,1,1,0.0f};
            CreateEmitterByType("Default", p);
            break;
        case 1: // Smoke
            p.name="Smoke"; p.texture="particle.png"; p.emitRate=40; p.burstCount=20; p.lifeMin=0.8f; p.lifeMax=1.6f; p.posMin={-1,0,-1}; p.posMax={1,0,1}; p.velMin={-0.5f,0.8f,-0.5f}; p.velMax={0.5f,1.5f,0.5f}; p.scaleStart={0.4f,0.4f,0.4f}; p.scaleEnd={0.9f,0.9f,0.9f}; p.colorStart={0.7f,0.7f,0.7f,0.6f}; p.colorEnd={1,1,1,0}; CreateEmitterByType("Primitive", p); break;
        case 2: // RingBurst
            p.name="Ring"; p.texture="gradationLine.png"; p.burstCount=32; p.lifeMin=0.6f; p.lifeMax=1.2f; p.scaleStart={0.8f,0.8f,1}; p.scaleEnd={1.2f,1.2f,1}; p.colorStart={0.8f,0.6f,1,0.9f}; p.colorEnd={1,0.9f,1,0}; CreateEmitterByType("Ring", p); break;
        case 3: // Fountain
            p.name="Fountain"; p.texture="gradationLine.png"; p.emitRate=25; p.lifeMin=0.8f; p.lifeMax=1.8f; p.posMin={-0.5f,0,-0.5f}; p.posMax={0.5f,0,0.5f}; p.velMin={-0.2f,1.5f,-0.2f}; p.velMax={0.2f,2.5f,0.2f}; p.scaleStart={0.4f,0.4f,0.4f}; p.scaleEnd={0.7f,0.7f,0.7f}; p.colorStart={0.6f,0.8f,1,0.7f}; p.colorEnd={0.9f,1,1,0}; CreateEmitterByType("Cylinder", p); break;
        case 4: // Radial
            p.name="Radial"; p.texture="particle.png"; p.burstCount=40; p.lifeMin=0.7f; p.lifeMax=1.4f; p.velMin={1,0,0}; p.velMax={3,0,0}; p.scaleStart={0.4f,0.4f,0.4f}; p.scaleEnd={0.2f,0.2f,0.2f}; p.colorStart={1,0.8f,0.5f,0.7f}; p.colorEnd={1,1,0.8f,0}; CreateEmitterByType("Original", p); break;
        case 5: // OrbitTrail
            p.name="OrbitTrail"; p.texture="particle.png"; p.emitRate=60; p.autoEmit=true; p.lifeMin=0.4f; p.lifeMax=0.8f; p.scaleStart={0.15f,0.15f,0.15f}; p.scaleEnd={0.05f,0.05f,0.05f}; p.colorStart={0.6f,0.8f,1,0.9f}; p.colorEnd={0.2f,0.4f,1,0}; CreateEmitterByType("OrbitTrail", p); break;
        case 6: // Aura（PlayerAura: AuraEmitter::Initialize が大半の値を上書きする）
            p.name="Aura"; p.texture="circle.png"; CreateEmitterByType("Aura", p); break;
        case 7: // HitEffect（横細・縦長の薄片を Z回転ランダムで8個 → 星型/剣撃。scale/回転は Primitive 既定値に任せる）
            p.name="HitEffect"; p.texture="circle.png"; p.maxInstances=64; p.burstCount=8; p.lifeMin=0.3f; p.lifeMax=0.5f; p.colorStart={1,1,1,1}; p.colorEnd={1,1,1,0}; CreateEmitterByType("Primitive", p); break;
        case 8: // Cylinder（縦ストリークの円筒。gradationLine テクスチャを側面に貼る）
            p.name="Cylinder"; p.texture="gradationLine.png"; p.maxInstances=8; p.burstCount=1; p.lifeMin=0.8f; p.lifeMax=1.2f; p.colorStart={1,1,1,1}; p.colorEnd={1,1,1,0}; CreateEmitterByType("Cylinder", p); break;
        }
    }
#endif
}

void TuboEngine::ParticleManager::DrawEmittersSection() {
#ifdef USE_IMGUI
    ImGui::Separator(); ImGui::Text("Emitters"); static int newType = 0;
    ImGui::Combo("New Type", &newType, "Default\0Primitive\0Ring\0Cylinder\0Original\0OrbitTrail\0Aura\0");

    static char nameBuf[64] = "Emitter";
    static char texBuf[128] = "particle.png";
    ImGui::InputText("Name", nameBuf, sizeof(nameBuf));
    ImGui::InputText("Texture", texBuf, sizeof(texBuf));

    if (ImGui::Button("Create")) {
        ParticlePreset p{};
        p.name = nameBuf;
        p.texture = texBuf;
        // other fields use defaults; ranges will be fixed by Initialize in each emitter type
        const char* types[] = {"Default","Primitive","Ring","Cylinder","Original","OrbitTrail","Aura"};
        CreateEmitterByType(types[newType], p);
    }

    ImGui::Separator();
    if (ImGui::Button("Save Selected") && !selectedEmitter_.empty()) { SaveSelected("Resources/Particles/" + selectedEmitter_ + ".json", {selectedEmitter_}); }
    ImGui::SameLine(); if (ImGui::Button("Load Merge Selected") && !selectedEmitter_.empty()) { pendingAction_=PendingActionType::LoadMergeSelected; pendingEmitterName_=selectedEmitter_; OpenConfirmPopup("ConfirmAction", ("Load & Merge '"+pendingEmitterName_+"' ?").c_str()); }
    ImGui::SameLine(); if (ImGui::Button("Save All") && !emitters_.empty()) { SaveAll("Resources/Particles/all.json"); }
    ImGui::SameLine(); if (ImGui::Button("Load All")) { pendingAction_=PendingActionType::LoadAll; OpenConfirmPopup("ConfirmAction", "Load All (overwrite current emitters)?"); }

    for (size_t i=0;i<emitters_.size();) {
        auto& e = emitters_[i]; auto& p = e->GetPreset(); bool open = ImGui::TreeNode(p.name.c_str()); if (!open) { ++i; continue; }
        selectedEmitter_ = p.name; uint32_t oldMax = p.maxInstances; ImGui::Text("Type: %s", DetectEmitterType(e.get()).c_str()); ImGui::Text("Instances: %u / %u", p.maxInstances, p.maxInstances);
        ImGui::Checkbox(("AutoEmit##"+p.name).c_str(), &p.autoEmit); ImGui::DragFloat(("EmitRate##"+p.name).c_str(), &p.emitRate, 0.1f, 0.0f, 400.0f); ImGui::DragInt(("Burst##"+p.name).c_str(), (int*)&p.burstCount, 1,1,5000); ImGui::DragInt(("MaxInstances##"+p.name).c_str(), (int*)&p.maxInstances, 1,1,500000);
        bool lifeEd=ImGui::DragFloat2(("LifeRange##"+p.name).c_str(), &p.lifeMin, 0.01f,0.01f,100.0f); bool posMinE=ImGui::DragFloat3(("PosMin##"+p.name).c_str(), &p.posMin.x,0.01f); bool posMaxE=ImGui::DragFloat3(("PosMax##"+p.name).c_str(), &p.posMax.x,0.01f);
        bool velMinE=ImGui::DragFloat3(("VelMin##"+p.name).c_str(), &p.velMin.x,0.01f); bool velMaxE=ImGui::DragFloat3(("VelMax##"+p.name).c_str(), &p.velMax.x,0.01f); bool sclStartE=ImGui::DragFloat3(("ScaleStart##"+p.name).c_str(), &p.scaleStart.x,0.01f); bool sclEndE=ImGui::DragFloat3(("ScaleEnd##"+p.name).c_str(), &p.scaleEnd.x,0.01f);
        bool colStartE=ImGui::ColorEdit4(("ColorStart##"+p.name).c_str(), &p.colorStart.x); bool colEndE=ImGui::ColorEdit4(("ColorEnd##"+p.name).c_str(), &p.colorEnd.x); bool gravE=ImGui::DragFloat3(("Gravity##"+p.name).c_str(), &p.gravity.x,0.01f); bool dragE=ImGui::DragFloat(("Drag##"+p.name).c_str(), &p.drag,0.001f,0.0f,2.0f);
        bool billboardE=ImGui::Checkbox(("Billboard##"+p.name).c_str(), &p.billboard); bool worldSpaceE=ImGui::Checkbox(("WorldSpace##"+p.name).c_str(), &p.simulateInWorldSpace); bool emitterPosE=ImGui::DragFloat3(("EmitterPos##"+p.name).c_str(), &p.emitterTransform.translate.x,0.01f);
        // Cylinder 専用: 縦軸まわりの回転速度（rad/秒, 0で停止, 符号で逆回転）
        if (auto* cyl = dynamic_cast<CylinderEmitter*>(e.get())) { float spin = cyl->GetSpinSpeedY(); if (ImGui::DragFloat(("SpinSpeedY##"+p.name).c_str(), &spin, 0.05f, -20.0f, 20.0f)) { cyl->SetSpinSpeedY(spin); MarkChanged(); } }
        if (lifeEd||posMinE||posMaxE||velMinE||velMaxE||sclStartE||sclEndE||colStartE||colEndE||gravE||dragE||billboardE||worldSpaceE||emitterPosE|| oldMax!=p.maxInstances) { FixPresetRanges(p); MarkChanged(); }
        // 再確保は描画記録前の Update() 側で安全に行う（ここで直接呼ぶとコマンドリストが
        // 参照中のバッファを解放して落ちるため、preset の値変更だけに留める）。
        if (oldMax!=p.maxInstances) { SetStatus("'%s' maxInstances=%u（次フレームのUpdateで再確保）", p.name.c_str(), p.maxInstances); }
        if (ImGui::Button(("Emit Burst##"+p.name).c_str())) { e->Emit(p.burstCount); MarkChanged(); }
        ImGui::SameLine(); if (ImGui::Button(("Clear##"+p.name).c_str())) { pendingAction_=PendingActionType::ClearEmitter; pendingEmitterName_=p.name; OpenConfirmPopup("ConfirmAction", ("Clear particles in '"+p.name+"' ?").c_str()); }
        ImGui::SameLine(); if (ImGui::Button(("Delete##"+p.name).c_str())) { SetStatus("Removed '%s'", p.name.c_str()); emitters_.erase(emitters_.begin()+i); if (selectedEmitter_==p.name) selectedEmitter_.clear(); MarkChanged(); ImGui::TreePop(); continue; }
        ImGui::TreePop(); ++i;
    }
    ImGui::Separator(); if (ImGui::Button("Undo")) { pendingAction_=PendingActionType::UndoAction; OpenConfirmPopup("ConfirmAction", "Undo last change?"); }
    ImGui::SameLine(); if (ImGui::Button("Redo")) { pendingAction_=PendingActionType::RedoAction; OpenConfirmPopup("ConfirmAction", "Redo next change?"); }
    if (ImGui::BeginPopupModal("ConfirmAction", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { ImGui::TextWrapped("%s", confirmMessage_.c_str()); ImGui::Separator(); if (ImGui::Button("OK", ImVec2(80,0))) { ExecutePendingAction(); ImGui::CloseCurrentPopup(); pendingAction_=PendingActionType::None; } ImGui::SameLine(); if (ImGui::Button("Cancel", ImVec2(80,0))) { pendingAction_=PendingActionType::None; ImGui::CloseCurrentPopup(); } ImGui::EndPopup(); }
#endif
}

void TuboEngine::ParticleManager::DrawImGui() {
#ifdef USE_IMGUI
    if (TuboEngine::ImGuiManager::GetInstance()->BeginPanel("Particle Manager")) {
        static int themeId = 0; ImGui::RadioButton("Dark", &themeId, 0); ImGui::SameLine(); ImGui::RadioButton("Light", &themeId, 1); ImGui::SameLine(); ImGui::RadioButton("HiContrast", &themeId, 2); static int lastTheme=-1; if (lastTheme!=themeId) { ApplyParticleManagerTheme(themeId); lastTheme=themeId; }
        ImGui::Separator(); DrawStatusBar(); DrawEmittersSection(); DrawTemplatesSection();
    }
    TuboEngine::ImGuiManager::GetInstance()->EndPanel();
#endif
}

IParticleEmitter* TuboEngine::ParticleManager::Find(const std::string& name) {
	for (auto& e : emitters_)
		if (e->GetName() == name)
			return e.get();
	return nullptr;
}
void TuboEngine::ParticleManager::Remove(const std::string& name) {
	emitters_.erase(std::remove_if(emitters_.begin(), emitters_.end(), [&](auto& u) { return u->GetName() == name; }), emitters_.end());
}

std::string TuboEngine::ParticleManager::BuildSnapshotJson() const {
    Json root;
    root["emitters"] = Json::array();
    for (const auto& e : emitters_) {
        root["emitters"].push_back(EmitterToJson(*e, DetectEmitterType(e.get())));
    }
    return root.dump();
}

void TuboEngine::ParticleManager::CaptureHistory() {
	std::string snap = BuildSnapshotJson();
	if (historyIndex_ + 1 < (int)history_.size())
		history_.erase(history_.begin() + historyIndex_ + 1, history_.end());
	history_.push_back(snap);
	historyIndex_ = (int)history_.size() - 1;
	if (history_.size() > 50) {
		history_.erase(history_.begin());
		--historyIndex_;
	}
}

void TuboEngine::ParticleManager::ApplySnapshot(const std::string& jsonStr) {
    // 復元中は MarkChanged を無視させる。
    // （CreateEmitterByType が MarkChanged を立てるため、復元そのものが「変更」として
    //   再記録され、Redo履歴を消してUndo/Redoが効かなくなるのを防ぐ）
    applyingSnapshot_ = true;
    Json root;
    if (!TuboEngine::JsonManager::Parse(jsonStr, root)) {
        SetStatus("Snapshot parse error");
        applyingSnapshot_ = false;
        return;
    }
    const Json* emitters = FindEmittersArray(root);
    if (!emitters) {
        SetStatus("Snapshot missing 'emitters'");
        applyingSnapshot_ = false;
        return;
    }
    emitters_.clear();
    for (const Json& j : *emitters) {
        CreateEmitterFromJson(*this, j);
    }
    SetStatus("Snapshot applied (%zu emitters)", emitters_.size());
    applyingSnapshot_ = false;
}

void TuboEngine::ParticleManager::Undo() {
	if (historyIndex_ <= 0 || history_.empty()) {
		SetStatus("Undo unavailable");
		return;
	}
	--historyIndex_;
	// フレーム途中で emitters を作り直すとコマンドリストが参照中のGPUリソースを破棄して
	// OBJECT_DELETED_WHILE_STILL_IN_USE で落ちる。実際の復元は Draw() 冒頭まで遅延する。
	pendingApplyJson_ = history_[historyIndex_];
	hasPendingApply_ = true;
}
void TuboEngine::ParticleManager::Redo() {
	if (historyIndex_ + 1 >= (int)history_.size()) {
		SetStatus("Redo unavailable");
		return;
	}
	++historyIndex_;
	pendingApplyJson_ = history_[historyIndex_];
	hasPendingApply_ = true;
}

void TuboEngine::ParticleManager::SaveAll(const std::string& path) {
    Json root;
    root["version"] = 2;
    root["snapshot"]["emitters"] = Json::array();
    for (const auto& e : emitters_) {
        root["snapshot"]["emitters"].push_back(EmitterToJson(*e, DetectEmitterType(e.get())));
    }
    if (!TuboEngine::JsonManager::Save(path, root)) {
        SetStatus("Save failed: %s", path.c_str());
        return;
    }
    SetStatus("Saved %zu emitters", emitters_.size());
}

void TuboEngine::ParticleManager::LoadAll(const std::string& path) {
    Json root;
    if (!TuboEngine::JsonManager::Load(path, root)) {
        SetStatus("Load failed: %s", path.c_str());
        return;
    }
    const Json* emitters = FindEmittersArray(root);
    if (!emitters) {
        SetStatus("Load: no emitters field");
        return;
    }
    emitters_.clear();
    for (const Json& j : *emitters) {
        CreateEmitterFromJson(*this, j);
    }
    SetStatus("Loaded %zu emitters", emitters_.size());
    MarkChanged();
}

void TuboEngine::ParticleManager::SaveSelected(const std::string& path, const std::vector<std::string>& names) {
    Json root;
    root["emitters"] = Json::array();
    for (const auto& name : names) {
        IParticleEmitter* e = Find(name);
        if (!e) {
            continue;
        }
        root["emitters"].push_back(EmitterToJson(*e, DetectEmitterType(e)));
    }
    if (!TuboEngine::JsonManager::Save(path, root)) {
        SetStatus("SaveSelected failed");
        return;
    }
    SetStatus("Saved %zu selected", root["emitters"].size());
}

void TuboEngine::ParticleManager::LoadMerge(const std::string& path) {
    Json root;
    if (!TuboEngine::JsonManager::Load(path, root)) {
        SetStatus("Merge load failed");
        return;
    }
    const Json* emitters = FindEmittersArray(root);
    if (!emitters) {
        SetStatus("Merge: no emitters");
        return;
    }
    size_t added = 0;
    for (const Json& j : *emitters) {
        if (CreateEmitterFromJson(*this, j)) {
            ++added;
        }
    }
    SetStatus("Merged %zu emitters", added);
    MarkChanged();
}

void TuboEngine::ParticleManager::InitialLoad(const std::string& filePath) {
    if (initialLoaded_)
        return;
    initialLoaded_ = true;
    // ファイルが無い・空のときは何も読まない（初回起動時など。エラーにはしない）
    std::error_code ec;
    const std::string resolved = TuboEngine::JsonManager::ResolvePath(filePath);
    if (!std::filesystem::exists(resolved, ec) || std::filesystem::file_size(resolved, ec) == 0) {
        SetStatus("Initial load skipped (%s not found or empty)", filePath.c_str());
        return;
    }
    LoadAll(filePath);
}

#ifdef USE_IMGUI
void TuboEngine::ParticleManager::OpenConfirmPopup(const char* popupName, const char* message) {
	confirmMessage_ = message ? message : "";
	ImGui::OpenPopup(popupName);
}
void TuboEngine::ParticleManager::ExecutePendingAction() {
    switch (pendingAction_) {
    case PendingActionType::DeleteEmitter: if (!pendingEmitterName_.empty()) { Remove(pendingEmitterName_); if (selectedEmitter_==pendingEmitterName_) selectedEmitter_.clear(); SetStatus("Removed '%s'", pendingEmitterName_.c_str()); MarkChanged(); } break;
    case PendingActionType::ClearEmitter: if (auto* e = Find(pendingEmitterName_)) { e->ClearAll(); SetStatus("Cleared '%s'", pendingEmitterName_.c_str()); MarkChanged(); } break;
    case PendingActionType::LoadAll: LoadAll("Resources/Particles/all.json"); selectedEmitter_.clear(); break;
    case PendingActionType::LoadMergeSelected: if (!pendingEmitterName_.empty()) LoadMerge("Resources/Particles/" + pendingEmitterName_ + ".json"); break;
    case PendingActionType::UndoAction: Undo(); break;
    case PendingActionType::RedoAction: Redo(); break; case PendingActionType::None: break; }
    pendingEmitterName_.clear(); pendingAction_ = PendingActionType::None; }
#endif


// プレビュー適用ヘルパー
void TuboEngine::ParticleManager::ApplyPreviewPreset(const ParticlePreset& src, int type) {
    if (!previewEnabled_) return;

    // 型変更や Texture 変更などで再生成が必要か判定
    if (previewEmitter_ == nullptr || previewType_ != type || previewCached_.texture != src.texture || previewCached_.billboard != src.billboard) {
        previewNeedsRecreate_ = true;
    }

    // プリセット差分のキャッシュ更新
    previewCached_ = src;

    if (previewNeedsRecreate_) {
        previewEmitter_.reset();
        switch (type) {
        case 0:
            previewEmitter_ = std::make_unique<DefaultEmitter>();
            break;
        case 1:
            previewEmitter_ = std::make_unique<PrimitiveEmitter>();
            break;
        case 2:
            previewEmitter_ = std::make_unique<RingEmitter>();
            break;
        case 3:
            previewEmitter_ = std::make_unique<CylinderEmitter>();
            break;
        case 4:
            previewEmitter_ = std::make_unique<OriginalEmitter>();
            break;
        case 5:
            previewEmitter_ = std::make_unique<OrbitTrailEmitter>();
            break;
        case 6:
            previewEmitter_ = std::make_unique<AuraEmitter>();
            break;
        default:
            previewEmitter_ = std::make_unique<PrimitiveEmitter>();
            break;
        }
        previewType_ = type;

        // コピーして初期化
        ParticlePreset initPreset = src;
        initPreset.name = "_Preview"; // 固定名
        previewEmitter_->Initialize(initPreset);
        previewNeedsRecreate_ = false;
    } else {
        // 既存のプリセットを上書き
        if (previewEmitter_) {
            auto& dst = previewEmitter_->GetPreset();
            dst = src;
            dst.name = "_Preview";
        }
    }
}

// プレビュー更新
void TuboEngine::ParticleManager::UpdatePreview(float dt, Camera* cam) {
    if (!previewEnabled_ || !previewEmitter_) return;
    auto& p = previewEmitter_->GetPreset();
    if (p.autoEmit && p.emitRate > 0.0f) {
        float interval = 1.0f / p.emitRate;
        p._emitAccum += dt;
        while (p._emitAccum >= interval) {
            previewEmitter_->Emit(1);
            p._emitAccum -= interval;
        }
    }
    previewEmitter_->Update(dt, cam);
}

// プレビュー描画
void TuboEngine::ParticleManager::DrawPreview(ID3D12GraphicsCommandList* cmd) {
    if (!previewEnabled_ || !previewEmitter_) return;
    previewEmitter_->Draw(cmd);
}
