#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>

///----------------------------------------------------
/// Line描画用のパイプラインステートオブジェクト管理クラス
///----------------------------------------------------
class LinePSO {
public:
    ///<summary>初期化処理</summary>
    void Initialize();
    ///<summary>描画設定を行う</summary>
    void DrawSettingsCommon();

private:
    ///<summary>ルートシグネイチャの作成</summary>
    void CreateRootSignature();
    ///<summary>グラフィックパイプラインステートオブジェクトの作成</summary>
    void CreateGraphicPipeline();

    ///-----------------------------------------------------
    /// ルートシグネチャ構築用メンバ
    ///-----------------------------------------------------
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

    ///-----------------------------------------------------
    /// 頂点レイアウト定義用メンバ
    ///-----------------------------------------------------
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};

    ///-----------------------------------------------------
    /// ブレンドステート定義用メンバ
    ///-----------------------------------------------------
    D3D12_BLEND_DESC blendDesc{};

    ///-----------------------------------------------------
    /// ラスタライザステート定義用メンバ
    ///-----------------------------------------------------
    D3D12_RASTERIZER_DESC rasterizerDesc{};

    ///-----------------------------------------------------
    /// シェーダ管理用メンバ
    ///-----------------------------------------------------
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob;
    Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob;

    ///-----------------------------------------------------
    /// 深度ステンシルステート定義用メンバ
    ///-----------------------------------------------------
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

    ///-----------------------------------------------------
    /// グラフィックスパイプラインステート管理用メンバ
    ///-----------------------------------------------------
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState = nullptr;

    ///-----------------------------------------------------
    /// DirectXデバイス・コマンドリスト
    ///-----------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
};