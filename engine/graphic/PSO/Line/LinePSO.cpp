#include "LinePSO.h"
#include "DirectXCommon.h"

///----------------------------------------------------
/// LinePSOの初期化処理
///----------------------------------------------------
void LinePSO::Initialize() {
    // DirectXデバイス取得
	device = TuboEngine::DirectXCommon::GetInstance()->GetDevice();
    // コマンドリスト取得
	commandList = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();
    // グラフィックパイプライン作成
    CreateGraphicPipeline();
}

///----------------------------------------------------
/// LinePSOの共通描画設定
///----------------------------------------------------
void LinePSO::DrawSettingsCommon() {
    // ルートシグネチャ設定
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    // パイプラインステート設定
    commandList->SetPipelineState(graphicsPipeLineState.Get());
    // プリミティブトポロジ設定（ラインリスト）
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
}

///----------------------------------------------------
/// ルートシグネチャの作成
///----------------------------------------------------
void LinePSO::CreateRootSignature() {
    // ルートパラメータの型設定（CBV）
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    // 頂点シェーダで使用
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    // シェーダレジスタ設定
    rootParameters[0].Descriptor.ShaderRegister = 0;
    // レジスタスペース設定
    rootParameters[0].Descriptor.RegisterSpace = 0;

    // ルートシグネチャ記述情報設定
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // ルートシグネチャのシリアライズ
    HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        // エラー時は例外送出
        throw std::runtime_error(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
    }
    // ルートシグネチャ生成
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    if (FAILED(hr)) {
        // エラー時は例外送出
        throw std::runtime_error("Failed to create root signature");
    }
}

///----------------------------------------------------
/// グラフィックパイプラインの作成
///----------------------------------------------------
void LinePSO::CreateGraphicPipeline() {
    // ルートシグネチャ作成
    CreateRootSignature();

    // 頂点レイアウト（POSITION）設定
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[0].InputSlot = 0;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[0].InstanceDataStepRate = 0;

    // 頂点レイアウト（COLOR）設定
    inputElementDescs[1].SemanticName = "COLOR";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[1].InputSlot = 0;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[1].InstanceDataStepRate = 0;

    // 頂点レイアウト記述設定
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // ブレンドステート設定
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // ラスタライザステート設定
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // 頂点シェーダのコンパイル
	vertexShaderBlob = TuboEngine::DirectXCommon::GetInstance()->CompileShader(L"Resources/Shaders/Line.VS.hlsl", L"vs_6_0");
    // ピクセルシェーダのコンパイル
	pixelShaderBlob = TuboEngine::DirectXCommon::GetInstance()->CompileShader(L"Resources/Shaders/Line.PS.hlsl", L"ps_6_0");

    // グラフィックパイプラインステート記述設定
    graphicPipelineStateDesc.pRootSignature = rootSignature.Get();
    graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
    graphicPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
    graphicPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
    graphicPipelineStateDesc.BlendState = blendDesc;
    graphicPipelineStateDesc.RasterizerState = rasterizerDesc;
    graphicPipelineStateDesc.NumRenderTargets = 1;
    graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    graphicPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    graphicPipelineStateDesc.SampleDesc.Count = 1;
    graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // 深度ステンシルステート設定
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // グラフィックスパイプラインステート生成
    HRESULT hr = device->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipeLineState));
    if (FAILED(hr)) {
        // エラー時は例外送出
        throw std::runtime_error("Failed to create graphics pipeline state");
    }
}