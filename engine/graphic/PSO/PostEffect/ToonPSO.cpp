#include "ToonPSO.h"
#include "DirectXCommon.h"

void ToonPSO::Initialize() {
	PostEffectPSOBase::Initialize();
	CreateGraphicPipeline();
}

void ToonPSO::CreateGraphicPipeline() {
	// 必要ならルートパラメータ拡張（今回はベースのまま）
	// シェーダーパスを指定してベースのCreateGraphicPipelineを呼ぶ
	PostEffectPSOBase::CreateGraphicPipeline(L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl", L"Resources/Shaders/PostEffect/Toon.PS.hlsl");
}

void ToonPSO::CreateRootSignature() {
    // ルートパラメータ: SRV2つ（t0:通常テクスチャ, t1:深度テクスチャ）, CBV1つ（b0:定数バッファ）
    D3D12_DESCRIPTOR_RANGE descriptorRanges[2] = {};
    // t0: gTexture
    descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[0].NumDescriptors = 1;
    descriptorRanges[0].BaseShaderRegister = 0;
    descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    // t1: gDepthTexture
    descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[1].NumDescriptors = 1;
    descriptorRanges[1].BaseShaderRegister = 1;
    descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[3] = {};
	// 0: SRV (t0) - gTexture
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    // 1: CBV (b0) - gMaterial
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  
    // 2: SRV (t1) - gDepthTexture
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRanges[1];
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // サンプラ2つ: s0(通常), s1(Point)
    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
    // s0: gSampler
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    // s1: gSamplerPoint
    staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister = 1;
    staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.pParameters = rootParameters;
    desc.NumParameters = _countof(rootParameters);
    desc.pStaticSamplers = staticSamplers;
    desc.NumStaticSamplers = _countof(staticSamplers);
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    assert(SUCCEEDED(hr));
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}
