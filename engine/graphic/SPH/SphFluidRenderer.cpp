#include "SphFluidRenderer.h"
#include <cassert>
#include <cstring>
#undef min
#undef max

using namespace Microsoft::WRL;

static inline auto* Dx()  { return TuboEngine::DirectXCommon::GetInstance(); }
static inline auto* Srv() { return TuboEngine::SrvManager::GetInstance(); }

// ============================================================
//  Initialize
// ============================================================
void SphFluidRenderer::Initialize(int screenW, int screenH) {
    screenW_ = screenW;
    screenH_ = screenH;

    CreateRTs(screenW, screenH);
    CreatePSOs();

    // cbuffers (UPLOAD, persistently mapped)
    auto MakeCB = [&](UINT sz, ComPtr<ID3D12Resource>& res) {
        res = Dx()->CreateBufferResource((sz + 255) & ~255u);
    };
    MakeCB(sizeof(DepthCB),  depthCBRes_);
    MakeCB(sizeof(BlurCB),   blurCBRes_);
    MakeCB(sizeof(ShadeCB),  shadeCBRes_);

    depthCBRes_->Map(0, nullptr, reinterpret_cast<void**>(&depthCBMap_));
    blurCBRes_ ->Map(0, nullptr, reinterpret_cast<void**>(&blurCBMap_));
    shadeCBRes_->Map(0, nullptr, reinterpret_cast<void**>(&shadeCBMap_));
}

// ============================================================
//  Finalize
// ============================================================
void SphFluidRenderer::Finalize() {
    if (depthCBMap_) { depthCBRes_->Unmap(0, nullptr); depthCBMap_ = nullptr; }
    if (blurCBMap_)  { blurCBRes_ ->Unmap(0, nullptr); blurCBMap_  = nullptr; }
    if (shadeCBMap_) { shadeCBRes_->Unmap(0, nullptr); shadeCBMap_ = nullptr; }
}

// ============================================================
//  CreateRTs — float 深度テクスチャ × 2 + DSV を作成
// ============================================================
void SphFluidRenderer::CreateRTs(int w, int h) {
    auto* device = Dx()->GetDevice().Get();

    // ---- RTV ヒープ (2 entries: depth + blur) ----
    rtvHeap_ = Dx()->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
    UINT rtvStep = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    fluidDepthRTV_.ptr = rtvHeap_->GetCPUDescriptorHandleForHeapStart().ptr;
    fluidBlurRTV_.ptr  = rtvHeap_->GetCPUDescriptorHandleForHeapStart().ptr + rtvStep;

    // ---- R32_FLOAT RT を 2 つ作成 ----
    auto MakeFloatRT = [&](ComPtr<ID3D12Resource>& res, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                            int& srvIdx) {
        D3D12_HEAP_PROPERTIES hp{ D3D12_HEAP_TYPE_DEFAULT };
        D3D12_RESOURCE_DESC   rd{};
        rd.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        rd.Width            = (UINT64)w;
        rd.Height           = (UINT)h;
        rd.DepthOrArraySize = 1;
        rd.MipLevels        = 1;
        rd.Format           = DXGI_FORMAT_R32_FLOAT;
        rd.Flags            = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        rd.SampleDesc.Count = 1;

        D3D12_CLEAR_VALUE cv{};
        cv.Format   = DXGI_FORMAT_R32_FLOAT;
        cv.Color[0] = 0; cv.Color[1] = 0; cv.Color[2] = 0; cv.Color[3] = 0;

        HRESULT hr = device->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &cv, IID_PPV_ARGS(&res));
        assert(SUCCEEDED(hr));

        device->CreateRenderTargetView(res.Get(), nullptr, rtvHandle);

        srvIdx = (int)Srv()->Allocate();
        Srv()->CreateSRVforTexture2D((uint32_t)srvIdx, res, DXGI_FORMAT_R32_FLOAT, 1);
    };

    MakeFloatRT(fluidDepthRT_, fluidDepthRTV_, fluidDepthSrv_);
    MakeFloatRT(fluidBlurRT_,  fluidBlurRTV_,  fluidBlurSrv_);

    // ---- DSV ヒープ (1 entry) + D32_FLOAT 深度バッファ ----
    dsvHeap_ = Dx()->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
    fluidDSV_ = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

    {
        D3D12_HEAP_PROPERTIES hp{ D3D12_HEAP_TYPE_DEFAULT };
        D3D12_RESOURCE_DESC   rd{};
        rd.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        rd.Width            = (UINT64)w;
        rd.Height           = (UINT)h;
        rd.DepthOrArraySize = 1;
        rd.MipLevels        = 1;
        rd.Format           = DXGI_FORMAT_D32_FLOAT;
        rd.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        rd.SampleDesc.Count = 1;

        D3D12_CLEAR_VALUE cv{};
        cv.Format              = DXGI_FORMAT_D32_FLOAT;
        cv.DepthStencil.Depth  = 1.0f;
        cv.DepthStencil.Stencil = 0;

        HRESULT hr = device->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &cv, IID_PPV_ARGS(&fluidDSVRes_));
        assert(SUCCEEDED(hr));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(fluidDSVRes_.Get(), &dsvDesc, fluidDSV_);
    }
}

// ============================================================
//  CreateRootSig
//  withSampler=false: [0] Root CBV, [1] SRV table  (深度パス用)
//  withSampler=true : [0] Root CBV, [1] SRV table, static sampler s0  (ブラー/シェード用)
// ============================================================
void SphFluidRenderer::CreateRootSig(bool withSampler,
                                      ComPtr<ID3D12RootSignature>& outRS) {
    auto* device = Dx()->GetDevice().Get();

    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors     = 1;
    srvRange.BaseShaderRegister = 0;   // t0
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[2]{};
    // [0] CBV b0 (inline)
    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    // [1] SRV t0 (descriptor table)
    params[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges   = &srvRange;
    params[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
    sampler.MaxLOD           = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister   = 0;   // s0
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters     = 2;
    rsDesc.pParameters       = params;
    rsDesc.NumStaticSamplers = withSampler ? 1 : 0;
    rsDesc.pStaticSamplers   = withSampler ? &sampler : nullptr;
    rsDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob, err;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err);
    if (FAILED(hr) && err) OutputDebugStringA((char*)err->GetBufferPointer());
    assert(SUCCEEDED(hr));

    hr = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                      IID_PPV_ARGS(&outRS));
    assert(SUCCEEDED(hr));
}

// ============================================================
//  CreateGraphicsPSO — グラフィクス PSO 作成ヘルパー
// ============================================================
void SphFluidRenderer::CreateGraphicsPSO(const wchar_t* vsPath, const wchar_t* psPath,
                                          DXGI_FORMAT rtFmt, bool depthEnable, bool alphaBlend,
                                          ID3D12RootSignature* rs,
                                          ComPtr<ID3D12PipelineState>& outPSO) {
    auto* device = Dx()->GetDevice().Get();

    ComPtr<IDxcBlob> vs = Dx()->CompileShader(vsPath, L"vs_6_0");
    ComPtr<IDxcBlob> ps = Dx()->CompileShader(psPath, L"ps_6_0");
    assert(vs && ps);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature      = rs;
    desc.VS                  = { vs->GetBufferPointer(), vs->GetBufferSize() };
    desc.PS                  = { ps->GetBufferPointer(), ps->GetBufferSize() };
    desc.InputLayout         = { nullptr, 0 };
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets    = 1;
    desc.RTVFormats[0]       = rtFmt;
    desc.SampleDesc.Count    = 1;
    desc.SampleMask          = D3D12_DEFAULT_SAMPLE_MASK;

    // ラスタライザ (深度パスはカリングなし)
    desc.RasterizerState                = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.CullMode       = depthEnable ? D3D12_CULL_MODE_NONE
                                                      : D3D12_CULL_MODE_BACK;

    // 深度ステンシル
    if (depthEnable) {
        desc.DepthStencilState.DepthEnable    = TRUE;
        desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
        desc.DSVFormat                        = DXGI_FORMAT_D32_FLOAT;
    } else {
        desc.DepthStencilState.DepthEnable    = FALSE;
        desc.DepthStencilState.StencilEnable  = FALSE;
    }

    // ブレンド
    if (alphaBlend) {
        auto& rt                        = desc.BlendState.RenderTarget[0];
        rt.BlendEnable                  = TRUE;
        rt.SrcBlend                     = D3D12_BLEND_SRC_ALPHA;
        rt.DestBlend                    = D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOp                      = D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha                = D3D12_BLEND_ONE;
        rt.DestBlendAlpha               = D3D12_BLEND_ZERO;
        rt.BlendOpAlpha                 = D3D12_BLEND_OP_ADD;
        rt.RenderTargetWriteMask        = D3D12_COLOR_WRITE_ENABLE_ALL;
    } else {
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    }

    HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&outPSO));
    assert(SUCCEEDED(hr));
}

// ============================================================
//  CreatePSOs
// ============================================================
void SphFluidRenderer::CreatePSOs() {
    CreateRootSig(false, depthRS_);
    CreateRootSig(true,  fsRS_);

    // 深度パス: billboard → R32_FLOAT, 深度テストあり, ブレンドなし
    CreateGraphicsPSO(
        L"Resources/Shaders/SPH/SphFluidDepth.VS.hlsl",
        L"Resources/Shaders/SPH/SphFluidDepth.PS.hlsl",
        DXGI_FORMAT_R32_FLOAT, true, false,
        depthRS_.Get(), depthPSO_);

    // ブラーパス: フルスクリーン → R32_FLOAT, 深度なし
    CreateGraphicsPSO(
        L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
        L"Resources/Shaders/SPH/SphFluidBlur.PS.hlsl",
        DXGI_FORMAT_R32_FLOAT, false, false,
        fsRS_.Get(), blurPSO_);

    // シェードパス: フルスクリーン → R8G8B8A8_UNORM_SRGB, alpha blend
    CreateGraphicsPSO(
        L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
        L"Resources/Shaders/SPH/SphFluidShade.PS.hlsl",
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, false, true,
        fsRS_.Get(), shadePSO_);
}

// ============================================================
//  Barrier ヘルパー
// ============================================================
void SphFluidRenderer::Barrier(ID3D12Resource* res,
                                 D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
    auto* cmd = Dx()->GetCommandList().Get();
    D3D12_RESOURCE_BARRIER b{};
    b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource   = res;
    b.Transition.StateBefore = from;
    b.Transition.StateAfter  = to;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &b);
}

// ============================================================
//  DrawDepthPass — billboard impostor を流体深度 RT に描画
// ============================================================
void SphFluidRenderer::DrawDepthPass(int instSrvIdx, int particleCount,
                                      const TuboEngine::Math::Matrix4x4& view,
                                      const TuboEngine::Math::Matrix4x4& proj) {
    if (!enabled || particleCount <= 0) return;

    auto* cmd = Dx()->GetCommandList().Get();
    auto* srv = Srv();

    // cbuffer 更新
    depthCBMap_->view           = view;
    depthCBMap_->proj           = proj;
    depthCBMap_->resolution[0]  = (float)screenW_;
    depthCBMap_->resolution[1]  = (float)screenH_;

    // RT を描画対象にセット、クリア
    cmd->OMSetRenderTargets(1, &fluidDepthRTV_, FALSE, &fluidDSV_);
    FLOAT clearColor[4] = {0, 0, 0, 0};
    cmd->ClearRenderTargetView(fluidDepthRTV_, clearColor, 0, nullptr);
    cmd->ClearDepthStencilView(fluidDSV_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // viewport / scissor
    D3D12_VIEWPORT vp = Dx()->GetViewport();
    D3D12_RECT     sc = Dx()->GetScissorRect();
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &sc);

    // PSO / Root sig
    cmd->SetGraphicsRootSignature(depthRS_.Get());
    cmd->SetPipelineState(depthPSO_.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ディスクリプタヒープ
    ID3D12DescriptorHeap* heaps[] = { srv->GetDescriptorHeap() };
    cmd->SetDescriptorHeaps(1, heaps);

    // Root [0] CBV, [1] instance SRV
    cmd->SetGraphicsRootConstantBufferView(0, depthCBRes_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(1, srv->GetGPUDescriptorHandle((uint32_t)instSrvIdx));

    // 描画: 6頂点/粒子 × 粒子数
    cmd->DrawInstanced(6, (UINT)particleCount, 0, 0);

    // fluidDepthRT_ → PSR に遷移 (ブラーパスが読む)
    Barrier(fluidDepthRT_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depthState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

// ============================================================
//  DrawBlurPass — バイラテラルブラーでスムージング
// ============================================================
void SphFluidRenderer::DrawBlurPass() {
    if (!enabled) return;

    auto* cmd = Dx()->GetCommandList().Get();
    auto* srv = Srv();

    // cbuffer 更新
    blurCBMap_->resolution[0] = (float)screenW_;
    blurCBMap_->resolution[1] = (float)screenH_;
    blurCBMap_->blurRadius    = params_.blurRadius;
    blurCBMap_->falloff       = params_.blurFalloff;

    // fluidBlurRT_ を描画対象にセット
    cmd->OMSetRenderTargets(1, &fluidBlurRTV_, FALSE, nullptr);
    FLOAT clear[4] = {0, 0, 0, 0};
    cmd->ClearRenderTargetView(fluidBlurRTV_, clear, 0, nullptr);

    D3D12_VIEWPORT vp = Dx()->GetViewport();
    D3D12_RECT     sc = Dx()->GetScissorRect();
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &sc);

    cmd->SetGraphicsRootSignature(fsRS_.Get());
    cmd->SetPipelineState(blurPSO_.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D12DescriptorHeap* heaps[] = { srv->GetDescriptorHeap() };
    cmd->SetDescriptorHeaps(1, heaps);

    cmd->SetGraphicsRootConstantBufferView(0, blurCBRes_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(1, srv->GetGPUDescriptorHandle((uint32_t)fluidDepthSrv_));
    cmd->DrawInstanced(3, 1, 0, 0);  // フルスクリーン三角形

    // 状態遷移
    Barrier(fluidBlurRT_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    blurState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    // fluidDepthRT_ を次フレーム用に RT へ戻す
    Barrier(fluidDepthRT_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    depthState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

// ============================================================
//  DrawShadePass — 法線再構築 + Fresnel シェーディング、alpha blend で合成
// ============================================================
void SphFluidRenderer::DrawShadePass(D3D12_CPU_DESCRIPTOR_HANDLE targetRTV,
                                      D3D12_CPU_DESCRIPTOR_HANDLE targetDSV,
                                      const TuboEngine::Math::Matrix4x4& view) {
    if (!enabled) return;

    auto* cmd = Dx()->GetCommandList().Get();
    auto* srv = Srv();

    // ライト方向をビュー空間へ変換 (w=0 で方向ベクトル)
    using V3 = TuboEngine::Math::Vector3;
    const V3& ld = params_.lightDir;
    float lx = ld.x * view.m[0][0] + ld.y * view.m[1][0] + ld.z * view.m[2][0];
    float ly = ld.x * view.m[0][1] + ld.y * view.m[1][1] + ld.z * view.m[2][1];
    float lz = ld.x * view.m[0][2] + ld.y * view.m[1][2] + ld.z * view.m[2][2];
    float llen = std::sqrt(lx*lx + ly*ly + lz*lz);
    if (llen > 1e-6f) { lx /= llen; ly /= llen; lz /= llen; }

    // cbuffer 更新
    shadeCBMap_->resolution[0]  = (float)screenW_;
    shadeCBMap_->resolution[1]  = (float)screenH_;
    shadeCBMap_->normalScale    = params_.normalScale;
    shadeCBMap_->specPower      = params_.specPower;
    shadeCBMap_->waterColor[0]  = params_.waterColor.x;
    shadeCBMap_->waterColor[1]  = params_.waterColor.y;
    shadeCBMap_->waterColor[2]  = params_.waterColor.z;
    shadeCBMap_->waterColor[3]  = params_.waterColor.w;
    shadeCBMap_->lightDir[0]    = lx;
    shadeCBMap_->lightDir[1]    = ly;
    shadeCBMap_->lightDir[2]    = lz;
    shadeCBMap_->fresnelBias    = params_.fresnelBias;

    // ターゲット RT を再セット (depth パスで変更されていたので戻す)
    cmd->OMSetRenderTargets(1, &targetRTV, FALSE, &targetDSV);

    D3D12_VIEWPORT vp = Dx()->GetViewport();
    D3D12_RECT     sc = Dx()->GetScissorRect();
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &sc);

    cmd->SetGraphicsRootSignature(fsRS_.Get());
    cmd->SetPipelineState(shadePSO_.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D12DescriptorHeap* heaps[] = { srv->GetDescriptorHeap() };
    cmd->SetDescriptorHeaps(1, heaps);

    cmd->SetGraphicsRootConstantBufferView(0, shadeCBRes_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(1, srv->GetGPUDescriptorHandle((uint32_t)fluidBlurSrv_));
    cmd->DrawInstanced(3, 1, 0, 0);  // フルスクリーン三角形

    // fluidBlurRT_ を次フレーム用に RT へ戻す
    Barrier(fluidBlurRT_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    blurState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
}
