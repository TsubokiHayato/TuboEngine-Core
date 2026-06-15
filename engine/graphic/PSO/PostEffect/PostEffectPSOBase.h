#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cassert>
#include <string>
#include <dxcapi.h>

class DirectXCommon;

class PostEffectPSOBase
{
public:
    virtual ~PostEffectPSOBase() = default;

    // 初期化
    virtual void Initialize();

    // グラフィックスパイプラインの作成
    virtual void CreateGraphicPipeline(
        const std::wstring& vsPath,
        const std::wstring& psPath);

    // 共通描画設定
    virtual void DrawSettingsCommon();

protected:
    // ルートシグネチャ作成（SRV1つ＋サンプラ1つのシンプルな例）
    virtual void CreateRootSignature();
protected:
   
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> commandList_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_;
    Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_;
};
