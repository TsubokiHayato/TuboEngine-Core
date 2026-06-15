#include "SphComputePipeline.h"
#include <cassert>
#include <cstring>
#include <cmath>
#include <algorithm>
#undef min
#undef max

using namespace Microsoft::WRL;

// ============================================================
//  Initialize
// ============================================================
void SphComputePipeline::Initialize(int particleCount,
                                     const TuboEngine::Math::Vector3& boundMin,
                                     const TuboEngine::Math::Vector3& boundMax,
                                     float cellSize, int maxPerCell) {
    particleCount_ = particleCount;
    auto* dx = TuboEngine::DirectXCommon::GetInstance();

    // ---- 空間ハッシュ次元を確定 (初期ボックスから算出、以後固定) ----
    cellSize_   = cellSize;
    gridMin_    = boundMin;
    maxPerCell_ = maxPerCell;
    gridDimX_ = std::max(1, (int)std::ceil((boundMax.x - boundMin.x) / cellSize));
    gridDimY_ = std::max(1, (int)std::ceil((boundMax.y - boundMin.y) / cellSize));
    gridDimZ_ = std::max(1, (int)std::ceil((boundMax.z - boundMin.z) / cellSize));
    numCells_ = gridDimX_ * gridDimY_ * gridDimZ_;

    // ---- 1. ルートシグネイチャ作成 ----
    CreateRootSignature();

    // ---- 2. Compute PSO ----
    CreateComputePSO(L"Resources/Shaders/SPH/CsDensity.hlsl",   psoDensity_);
    CreateComputePSO(L"Resources/Shaders/SPH/CsForce.hlsl",     psoForce_);
    CreateComputePSO(L"Resources/Shaders/SPH/CsIntegrate.hlsl", psoIntegrate_);
    CreateComputePSO(L"Resources/Shaders/SPH/CsPrepareInstances.hlsl", psoPrepare_);
    CreateComputePSO(L"Resources/Shaders/SPH/CsClearGrid.hlsl", psoClearGrid_);
    CreateComputePSO(L"Resources/Shaders/SPH/CsBuildGrid.hlsl", psoBuildGrid_);

    // ---- 3. 粒子バッファ (DEFAULT heap, UAV) ----
    CreateDefaultBuffer(sizeof(SphParticle) * particleCount, particleBuf_);

    // ---- 4. インスタンシングバッファ (DEFAULT heap, UAV + SRV) ----
    CreateDefaultBuffer(sizeof(SphGPUInstance) * particleCount, instanceBuf_);

    // ---- 5. 空間ハッシュバッファ ----
    CreateDefaultBuffer(sizeof(int) * (UINT64)numCells_, gridCountsBuf_);
    CreateDefaultBuffer(sizeof(int) * (UINT64)numCells_ * maxPerCell_, gridCellsBuf_);

    // ---- 6. ディスクリプタ割り当て ----
    auto* srv = TuboEngine::SrvManager::GetInstance();

    particleUavIndex_ = srv->Allocate();
    srv->CreateUAVForStructuredBuffer(
        particleUavIndex_, particleBuf_.Get(),
        particleCount, sizeof(SphParticle));

    instanceUavIndex_ = srv->Allocate();
    srv->CreateUAVForStructuredBuffer(
        instanceUavIndex_, instanceBuf_.Get(),
        particleCount, sizeof(SphGPUInstance));

    instancingSrvIndex_ = srv->Allocate();
    srv->CreateSRVForStructuredBuffer(
        instancingSrvIndex_, instanceBuf_.Get(),
        particleCount, sizeof(SphGPUInstance));

    gridCountsUav_ = srv->Allocate();
    srv->CreateUAVForStructuredBuffer(
        gridCountsUav_, gridCountsBuf_.Get(),
        numCells_, sizeof(int));

    gridCellsUav_ = srv->Allocate();
    srv->CreateUAVForStructuredBuffer(
        gridCellsUav_, gridCellsBuf_.Get(),
        numCells_ * maxPerCell_, sizeof(int));

    // ---- 7. パラメーター定数バッファ (UPLOAD) ----
    CreateParamsBuffer();

    initialized_ = true;
}

// ============================================================
//  Finalize
// ============================================================
void SphComputePipeline::Finalize() {
    if (paramsMapped_ && paramsCbuf_) {
        paramsCbuf_->Unmap(0, nullptr);
        paramsMapped_ = nullptr;
    }
    psoDensity_.Reset(); psoForce_.Reset();
    psoIntegrate_.Reset(); psoPrepare_.Reset();
    psoClearGrid_.Reset(); psoBuildGrid_.Reset();
    particleBuf_.Reset(); instanceBuf_.Reset(); paramsCbuf_.Reset();
    gridCountsBuf_.Reset(); gridCellsBuf_.Reset();
    rootSig_.Reset();
    initialized_ = false;
}

// ============================================================
//  UploadInitialParticles — 初期データを GPU に転送
// ============================================================
void SphComputePipeline::UploadInitialParticles(const std::vector<SphParticle>& particles) {
    if (!initialized_ || particles.empty()) return;

    auto* dx      = TuboEngine::DirectXCommon::GetInstance();
    auto* device  = dx->GetDevice().Get();
    auto* cmd     = dx->GetCommandList().Get();

    const UINT64 dataSize = sizeof(SphParticle) * particles.size();

    // ステージングバッファ (UPLOAD heap)
    ComPtr<ID3D12Resource> staging;
    D3D12_HEAP_PROPERTIES uploadHeap{};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{};
    bufDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width            = dataSize;
    bufDesc.Height           = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels        = 1;
    bufDesc.Format           = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&staging));
    assert(SUCCEEDED(hr));

    void* mapped = nullptr;
    staging->Map(0, nullptr, &mapped);
    std::memcpy(mapped, particles.data(), dataSize);
    staging->Unmap(0, nullptr);

    // DEFAULT heap バッファへコピー
    // 初回は COMMON 状態、2回目以降 (Reset) は Dispatch 後の UNORDERED_ACCESS 状態から遷移する
    D3D12_RESOURCE_STATES stateBefore = particleBufUploaded_
        ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        : D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = particleBuf_.Get();
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &barrier);

    cmd->CopyBufferRegion(particleBuf_.Get(), 0, staging.Get(), 0, dataSize);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    cmd->ResourceBarrier(1, &barrier);
    particleBufUploaded_ = true;

    // ステージングバッファは CommandExecution 後に解放されるよう保持
    // (ここでは CommandExecution 後まで生き続けるよう dx 側に任せる)
    // 簡易的に: Execute → Wait してから破棄
    dx->CommandExecution();
    // staging はここで破棄 (ComPtr がスコープを抜ける際に自動解放)
}

// ============================================================
//  Dispatch — 1フレーム分の SPH 計算
// ============================================================
void SphComputePipeline::Dispatch(const SphGpuParams& params, int substeps) {
    if (!initialized_) return;

    auto* cmd  = TuboEngine::DirectXCommon::GetInstance()->GetCommandList().Get();
    auto* srv  = TuboEngine::SrvManager::GetInstance();

    // instanceBuf_ が前フレームの描画で SRV 状態になっていれば UAV に戻す
    if (instanceBufInSRV_) {
        TransitionBarrier(cmd, instanceBuf_.Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        instanceBufInSRV_ = false;
    }

    // パラメーターを定数バッファに書き込む
    // gridMin は現在の boundMin に追従させる (境界移動でグリッドがズレるのを防ぐ)
    // gridDim はバッファ確保サイズ上限でキャップ (境界拡大しすぎた場合に安全に劣化)
    SphGpuParams p = params;
    p.gridMinX   = params.boundMinX;
    p.gridMinY   = params.boundMinY;
    p.gridMinZ   = params.boundMinZ;
    p.cellSize   = cellSize_;
    p.gridDimX   = std::min(gridDimX_,
                       std::max(1, (int)std::ceil((params.boundMaxX - params.boundMinX) / cellSize_)));
    p.gridDimY   = std::min(gridDimY_,
                       std::max(1, (int)std::ceil((params.boundMaxY - params.boundMinY) / cellSize_)));
    p.gridDimZ   = std::min(gridDimZ_,
                       std::max(1, (int)std::ceil((params.boundMaxZ - params.boundMinZ) / cellSize_)));
    p.maxPerCell = maxPerCell_;
    std::memcpy(paramsMapped_, &p, sizeof(SphGpuParams));

    // ディスクリプタヒープをセット
    ID3D12DescriptorHeap* heaps[] = { srv->GetDescriptorHeap() };
    cmd->SetDescriptorHeaps(1, heaps);

    // Compute ルートシグネイチャ + リソースをセット
    cmd->SetComputeRootSignature(rootSig_.Get());
    cmd->SetComputeRootConstantBufferView(0, paramsCbuf_->GetGPUVirtualAddress());
    cmd->SetComputeRootDescriptorTable(1, srv->GetGPUDescriptorHandle(particleUavIndex_));
    cmd->SetComputeRootDescriptorTable(2, srv->GetGPUDescriptorHandle(instanceUavIndex_));
    cmd->SetComputeRootDescriptorTable(3, srv->GetGPUDescriptorHandle(gridCountsUav_));
    cmd->SetComputeRootDescriptorTable(4, srv->GetGPUDescriptorHandle(gridCellsUav_));

    const UINT pGroups = (particleCount_ + 255) / 256;  // 粒子数ベース
    const UINT cGroups = (numCells_     + 255) / 256;    // セル数ベース

    for (int s = 0; s < substeps; ++s) {
        // ---- 空間ハッシュ構築: カウンタクリア → 粒子登録 ----
        // substep 毎に位置が変わるため毎回再構築する
        cmd->SetPipelineState(psoClearGrid_.Get());
        cmd->Dispatch(cGroups, 1, 1);
        UAVBarrier(cmd, gridCountsBuf_.Get());

        cmd->SetPipelineState(psoBuildGrid_.Get());
        cmd->Dispatch(pGroups, 1, 1);
        UAVBarrier(cmd, gridCountsBuf_.Get());
        UAVBarrier(cmd, gridCellsBuf_.Get());

        // ---- Density pass ----
        cmd->SetPipelineState(psoDensity_.Get());
        cmd->Dispatch(pGroups, 1, 1);
        UAVBarrier(cmd, particleBuf_.Get());

        // ---- Force pass ----
        cmd->SetPipelineState(psoForce_.Get());
        cmd->Dispatch(pGroups, 1, 1);
        UAVBarrier(cmd, particleBuf_.Get());

        // ---- Integrate pass ----
        cmd->SetPipelineState(psoIntegrate_.Get());
        cmd->Dispatch(pGroups, 1, 1);
        UAVBarrier(cmd, particleBuf_.Get());
    }

    // ---- PrepareInstances: WVP を GPU 上で計算 ----
    cmd->SetPipelineState(psoPrepare_.Get());
    cmd->Dispatch(pGroups, 1, 1);
    UAVBarrier(cmd, instanceBuf_.Get());

    // instanceBuf_ を UAV → NON_PIXEL_SHADER_RESOURCE に遷移
    // → Object3DDraw パスで VS が StructuredBuffer<InstanceData> として読める
    TransitionBarrier(cmd, instanceBuf_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    instanceBufInSRV_ = true;
}

// ============================================================
//  private: CreateRootSignature
//  [0] CBV b0  — SphParams
//  [1] UAV u0  — particle RWStructuredBuffer
//  [2] UAV u1  — instance RWStructuredBuffer
// ============================================================
void SphComputePipeline::CreateRootSignature() {
    auto* device = TuboEngine::DirectXCommon::GetInstance()->GetDevice().Get();

    D3D12_DESCRIPTOR_RANGE uavRange0{};
    uavRange0.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange0.NumDescriptors     = 1;
    uavRange0.BaseShaderRegister = 0;  // u0
    uavRange0.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange1{};
    uavRange1.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange1.NumDescriptors     = 1;
    uavRange1.BaseShaderRegister = 1;  // u1
    uavRange1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange2{};
    uavRange2.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange2.NumDescriptors     = 1;
    uavRange2.BaseShaderRegister = 2;  // u2 (grid counts)
    uavRange2.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange3{};
    uavRange3.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange3.NumDescriptors     = 1;
    uavRange3.BaseShaderRegister = 3;  // u3 (grid cells)
    uavRange3.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[5]{};
    // [0] CBV b0
    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    // [1] UAV u0
    params[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges   = &uavRange0;
    params[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
    // [2] UAV u1
    params[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 1;
    params[2].DescriptorTable.pDescriptorRanges   = &uavRange1;
    params[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
    // [3] UAV u2 (grid counts)
    params[3].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[3].DescriptorTable.NumDescriptorRanges = 1;
    params[3].DescriptorTable.pDescriptorRanges   = &uavRange2;
    params[3].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
    // [4] UAV u3 (grid cells)
    params[4].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[4].DescriptorTable.NumDescriptorRanges = 1;
    params[4].DescriptorTable.pDescriptorRanges   = &uavRange3;
    params[4].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 5;
    rsDesc.pParameters   = params;
    rsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;  // Compute は IA 不要

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                            &sigBlob, &errBlob);
    assert(SUCCEEDED(hr));
    hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(),
                                      sigBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&rootSig_));
    assert(SUCCEEDED(hr));
}

// ============================================================
//  private: CreateComputePSO
// ============================================================
void SphComputePipeline::CreateComputePSO(const wchar_t* shaderPath,
                                            ComPtr<ID3D12PipelineState>& outPso) {
    auto* dx     = TuboEngine::DirectXCommon::GetInstance();
    auto* device = dx->GetDevice().Get();

    ComPtr<IDxcBlob> csBlob = dx->CompileShader(shaderPath, L"cs_6_0");
    assert(csBlob);

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSig_.Get();
    desc.CS             = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

    HRESULT hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&outPso));
    assert(SUCCEEDED(hr));
}

// ============================================================
//  private: CreateDefaultBuffer — UAV フラグ付き DEFAULT heap バッファ
// ============================================================
void SphComputePipeline::CreateDefaultBuffer(UINT64 size,
                                               ComPtr<ID3D12Resource>& outBuf) {
    auto* device = TuboEngine::DirectXCommon::GetInstance()->GetDevice().Get();

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width            = size;
    resDesc.Height           = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels        = 1;
    resDesc.Format           = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    // バッファは常に COMMON で生成される（D3D12 仕様）
    // UAV アクセス時に COMMON → UNORDERED_ACCESS へ暗黙プロモーションされる
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &resDesc, D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&outBuf));
    assert(SUCCEEDED(hr));
}

// ============================================================
//  private: CreateParamsBuffer — UPLOAD heap 定数バッファ
// ============================================================
void SphComputePipeline::CreateParamsBuffer() {
    auto* dx = TuboEngine::DirectXCommon::GetInstance();
    // サイズは 256 バイト境界にアライメント
    const UINT size = (sizeof(SphGpuParams) + 255) & ~255u;
    paramsCbuf_ = dx->CreateBufferResource(size);
    paramsCbuf_->Map(0, nullptr, reinterpret_cast<void**>(&paramsMapped_));
}

// ============================================================
//  private: UAVBarrier
// ============================================================
void SphComputePipeline::UAVBarrier(ID3D12GraphicsCommandList* cmd,
                                      ID3D12Resource* resource) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource;
    cmd->ResourceBarrier(1, &barrier);
}

// ============================================================
//  private: TransitionBarrier
// ============================================================
void SphComputePipeline::TransitionBarrier(ID3D12GraphicsCommandList* cmd,
                                             ID3D12Resource* resource,
                                             D3D12_RESOURCE_STATES before,
                                             D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter  = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &barrier);
}
