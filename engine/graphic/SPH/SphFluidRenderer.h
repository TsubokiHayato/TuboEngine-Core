#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Vector4.h"
#include <d3d12.h>
#include <wrl/client.h>
#undef min
#undef max

/// @brief Screen Space Fluid Rendering (3 パス)
///
/// Pass 1: billboard 深度  → R32_FLOAT テクスチャ (球面インポスター)
/// Pass 2: バイラテラルブラー → スムージング済み深度テクスチャ
/// Pass 3: 法線再構築 + Fresnel シェーディング → alpha blend でシーン上に合成
///
/// 呼び出し順:
///   DrawDepthPass()  ← OffscreenRT が bound されている間
///   DrawBlurPass()
///   DrawShadePass(offscreenRTV, dsv)  ← offscreenRT を書き込み先に戻す
class SphFluidRenderer {
public:
    struct Params {
        float blurRadius   = 3.0f;   // ブラーカーネル半径 (1-5)
        float blurFalloff  = 1.0f;   // バイラテラル深度重み
        float normalScale  = 50.0f;  // 法線増幅率
        float specPower    = 64.0f;  // 鏡面反射の鋭さ
        float fresnelBias  = 0.1f;   // フレネル最小値
        TuboEngine::Math::Vector4 waterColor = {0.1f, 0.4f, 0.7f, 1.0f};
        TuboEngine::Math::Vector3 lightDir   = {0.5f,-1.0f, 0.5f};
    };

    void Initialize(int screenW, int screenH);
    void Finalize();

    void DrawDepthPass(int instSrvIdx, int particleCount,
                       const TuboEngine::Math::Matrix4x4& view,
                       const TuboEngine::Math::Matrix4x4& proj);
    void DrawBlurPass();
    void DrawShadePass(D3D12_CPU_DESCRIPTOR_HANDLE targetRTV,
                       D3D12_CPU_DESCRIPTOR_HANDLE targetDSV,
                       const TuboEngine::Math::Matrix4x4& view);

    Params& GetParams() { return params_; }
    bool    enabled    = true;

private:
    void CreateRTs(int w, int h);
    void CreatePSOs();
    void CreateRootSig(bool withSampler, Microsoft::WRL::ComPtr<ID3D12RootSignature>& outRS);
    void CreateGraphicsPSO(const wchar_t* vsPath, const wchar_t* psPath,
                           DXGI_FORMAT rtFmt, bool depthEnable, bool alphaBlend,
                           ID3D12RootSignature* rs,
                           Microsoft::WRL::ComPtr<ID3D12PipelineState>& outPSO);
    void Barrier(ID3D12Resource* res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

    // ---- 深度 RT (R32_FLOAT) ----
    Microsoft::WRL::ComPtr<ID3D12Resource> fluidDepthRT_;
    D3D12_CPU_DESCRIPTOR_HANDLE            fluidDepthRTV_{};
    int                                    fluidDepthSrv_ = -1;
    D3D12_RESOURCE_STATES                  depthState_    = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // ---- ブラー RT (R32_FLOAT) ----
    Microsoft::WRL::ComPtr<ID3D12Resource> fluidBlurRT_;
    D3D12_CPU_DESCRIPTOR_HANDLE            fluidBlurRTV_{};
    int                                    fluidBlurSrv_  = -1;
    D3D12_RESOURCE_STATES                  blurState_     = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // ---- 深度パス用 DSV (D32_FLOAT) ----
    Microsoft::WRL::ComPtr<ID3D12Resource>       fluidDSVRes_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE                  fluidDSV_{};

    // ---- 独自 RTV ヒープ (fluidDepth + fluidBlur) ----
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;

    // ---- PSO / Root Sig ----
    Microsoft::WRL::ComPtr<ID3D12RootSignature>  depthRS_;  // 深度パス用 (サンプラなし)
    Microsoft::WRL::ComPtr<ID3D12RootSignature>  fsRS_;     // ブラー/シェード用 (サンプラあり)
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  depthPSO_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  blurPSO_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  shadePSO_;

    // ---- cbuffers (UPLOAD heap, persistently mapped) ----
    struct alignas(16) DepthCB {
        TuboEngine::Math::Matrix4x4 view;
        TuboEngine::Math::Matrix4x4 proj;
        float resolution[2];
        float _pad[2];
    };
    struct alignas(16) BlurCB {
        float resolution[2];
        float blurRadius;
        float falloff;
    };
    struct alignas(16) ShadeCB {
        float resolution[2];
        float normalScale;
        float specPower;
        float waterColor[4];
        float lightDir[3];
        float fresnelBias;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> depthCBRes_, blurCBRes_, shadeCBRes_;
    DepthCB* depthCBMap_ = nullptr;
    BlurCB*  blurCBMap_  = nullptr;
    ShadeCB* shadeCBMap_ = nullptr;

    Params params_;
    int    screenW_ = 0, screenH_ = 0;
};
