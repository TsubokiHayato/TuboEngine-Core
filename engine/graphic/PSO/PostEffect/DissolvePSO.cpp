#include "DissolvePSO.h"

void DissolvePSO::Initialize() {

	PostEffectPSOBase::Initialize();
	CreateGraphicPipeline();
}
void DissolvePSO::CreateGraphicPipeline() {
	// シェーダーパスを指定してベースのCreateGraphicPipelineを呼ぶ
	PostEffectPSOBase::CreateGraphicPipeline(
		L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
		L"Resources/Shaders/PostEffect/Dissolve.PS.hlsl"
	);
}



void DissolvePSO::CreateRootSignature() {
    // ※ 以前は param0 を「t0,t1 の2連続テーブル」にして slot0 起点で読んでいたが、
    //    重ねがけ（ping-pong）で入力(t0)のスロットが動くと t1(マスク) が破綻する。
    //    そこでマスク(t1)を専用の param2 で明示バインドする方式に変更した。
    D3D12_ROOT_PARAMETER rootParameters[3] = {};

    // t0: gTexture（入力）
    D3D12_DESCRIPTOR_RANGE srvRangeTex = {};
    srvRangeTex.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRangeTex.NumDescriptors = 1;
    srvRangeTex.BaseShaderRegister = 0;
    srvRangeTex.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    // t1: gMaskTexture（マスク）
    D3D12_DESCRIPTOR_RANGE srvRangeMask = {};
    srvRangeMask.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRangeMask.NumDescriptors = 1;
    srvRangeMask.BaseShaderRegister = 1;
    srvRangeMask.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // 0: SRV (t0) - gTexture（入力。マネージャがバインド）
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRangeTex;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 1: CBV
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 2: SRV (t1) - gMaskTexture（Dissolve側で明示バインド）
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable.pDescriptorRanges = &srvRangeMask;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // サンプラ
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.pParameters = rootParameters;
    desc.NumParameters = _countof(rootParameters);
    desc.pStaticSamplers = &sampler;
    desc.NumStaticSamplers = 1;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    assert(SUCCEEDED(hr));
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}
