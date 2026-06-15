#include "InstancedMeshRenderer.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "TransformationMatrix.h"
#include <cassert>
#include <cstring>
#undef min
#undef max

// ============================================================
//  Initialize
// ============================================================
void InstancedMeshRenderer::Initialize(const std::string& modelPath,
                                        int maxInstances,
                                        TuboEngine::Camera* camera)
{
    auto* dx = TuboEngine::DirectXCommon::GetInstance();

    // ---- テンプレート Object3d (ライト・カメラ CBV 管理用) ----
    obj_ = std::make_unique<TuboEngine::Object3d>();
    obj_->Initialize(modelPath);
    obj_->SetCamera(camera);
    obj_->SetLightType(1);  // Phong

    // ---- CommonData CBV : useInstancing = 1 ----
    commonBuf_ = dx->CreateBufferResource(sizeof(int32_t) * 4); // 16 byte align
    int32_t* commonMapped = nullptr;
    commonBuf_->Map(0, nullptr, reinterpret_cast<void**>(&commonMapped));
    *commonMapped = 1;  // useInstancing = true
    commonBuf_->Unmap(0, nullptr);

    // ---- Dummy TransformMatrix CBV (root param[1]) ----
    dummyTransformBuf_ = dx->CreateBufferResource(sizeof(TuboEngine::TransformationMatrix));
    TuboEngine::TransformationMatrix* dummyMapped = nullptr;
    dummyTransformBuf_->Map(0, nullptr, reinterpret_cast<void**>(&dummyMapped));
    dummyMapped->WVP   = TuboEngine::Math::MakeIdentity4x4();
    dummyMapped->World = TuboEngine::Math::MakeIdentity4x4();
    dummyTransformBuf_->Unmap(0, nullptr);
}

// ============================================================
//  Update — ライト・カメラ CBV を毎フレーム更新
// ============================================================
void InstancedMeshRenderer::Update(TuboEngine::Camera* camera) {
    if (!obj_) return;
    obj_->SetCamera(camera);
    obj_->Update();
}

// ============================================================
//  Draw — 1 DrawCall でインスタンシング描画
//
//  Root parameter layout (PSO.cpp より):
//    [0]  PS b0  : Material          ← Model::DrawInstanced() がセット
//    [1]  VS b0  : TransformMatrix   ← dummy (useInstancing=1 なら VS は無視)
//    [2]  PS t0  : Texture           ← Model::DrawInstanced() がセット
//    [3]  PS t1  : CubeTexture
//    [4]  PS b1  : DirectionalLight
//    [5]  PS b2  : Camera
//    [6]  PS b3  : LightType
//    [7]  PS b4  : PointLight
//    [8]  PS b5  : SpotLight
//    [9]  VS t0  : InstanceData SRV  ← Model::DrawInstanced() がセット
//    [10] VS b1  : CommonData        ← Model::DrawInstanced() がセット
// ============================================================
void InstancedMeshRenderer::Draw(D3D12_GPU_VIRTUAL_ADDRESS instanceDataGpuAddr,
                                   uint32_t instanceCount)
{
    if (!obj_ || instanceCount == 0) return;

    auto* cmd = TuboEngine::DirectXCommon::GetInstance()->GetCommandList().Get();
    const std::string cubeMap = obj_->GetCubeMapFilePath();

    // [1] TransformMatrix (dummy, VS b0)
    cmd->SetGraphicsRootConstantBufferView(1, dummyTransformBuf_->GetGPUVirtualAddress());

    // [3] CubeTexture (PS t1)
    cmd->SetGraphicsRootDescriptorTable(
        3, TuboEngine::TextureManager::GetInstance()->GetSrvHandleGPU(cubeMap));

    // [4] DirectionalLight (PS b1)
    cmd->SetGraphicsRootConstantBufferView(
        4, obj_->GetDirectionalLightResource()->GetGPUVirtualAddress());

    // [5] Camera (PS b2)
    cmd->SetGraphicsRootConstantBufferView(
        5, obj_->GetCameraForGPUResource()->GetGPUVirtualAddress());

    // [6] LightType (PS b3)
    cmd->SetGraphicsRootConstantBufferView(
        6, obj_->GetLightTypeResource()->GetGPUVirtualAddress());

    // [7] PointLight (PS b4)
    cmd->SetGraphicsRootConstantBufferView(
        7, obj_->GetPointLightResource()->GetGPUVirtualAddress());

    // [8] SpotLight (PS b5)
    cmd->SetGraphicsRootConstantBufferView(
        8, obj_->GetSpotLightResource()->GetGPUVirtualAddress());

    // [0][2][9][10] + DrawInstanced は Model::DrawInstanced() が担当
    obj_->GetModel()->DrawInstanced(
        instanceCount,
        instanceDataGpuAddr,
        commonBuf_->GetGPUVirtualAddress());
}

// ============================================================
//  Finalize
// ============================================================
void InstancedMeshRenderer::Finalize() {
    obj_.reset();
    commonBuf_.Reset();
    dummyTransformBuf_.Reset();
}
