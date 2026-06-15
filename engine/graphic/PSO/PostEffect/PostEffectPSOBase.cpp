#include "PostEffectPSOBase.h"
#include "DirectXCommon.h"

void PostEffectPSOBase::Initialize() {
	
	device_ = TuboEngine::DirectXCommon::GetInstance()->GetDevice();
	commandList_ = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();
}

void PostEffectPSOBase::CreateGraphicPipeline(const std::wstring& vsPath, const std::wstring& psPath) {
	CreateRootSignature();

	// シェーダーのコンパイル
	vertexShaderBlob_ = TuboEngine::DirectXCommon::GetInstance()->CompileShader(vsPath, L"vs_6_0");
	pixelShaderBlob_ = TuboEngine::DirectXCommon::GetInstance()->CompileShader(psPath, L"ps_6_0");
	assert(vertexShaderBlob_ && pixelShaderBlob_);

	// PSO設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = rootSignature_.Get();
	desc.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() };
	desc.PS = { pixelShaderBlob_->GetBufferPointer(), pixelShaderBlob_->GetBufferSize() };
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.DepthStencilState.DepthEnable = FALSE;
	desc.DepthStencilState.StencilEnable = FALSE;
	desc.InputLayout = { nullptr, 0 };
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&graphicsPipelineState_));
	assert(SUCCEEDED(hr));
}


void PostEffectPSOBase::DrawSettingsCommon() {
	commandList_->SetGraphicsRootSignature(rootSignature_.Get());
	commandList_->SetPipelineState(graphicsPipelineState_.Get());
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void PostEffectPSOBase::CreateRootSignature() {
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.pDescriptorRanges = &range;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    desc.pParameters = &param;
    desc.NumParameters = 1;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    desc.pStaticSamplers = &sampler;
    desc.NumStaticSamplers = 1;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        assert(false);
    }
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

