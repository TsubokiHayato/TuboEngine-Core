#include "BloomPSO.h"

void BloomPSO::Initialize() {

	PostEffectPSOBase::Initialize();
	CreateGraphicPipeline();
}

void BloomPSO::CreateGraphicPipeline() {
	// 必要ならルートパラメータ拡張（今回はベースのまま）
	// シェーダーパスを指定してベースのCreateGraphicPipelineを呼ぶ
	PostEffectPSOBase::CreateGraphicPipeline(
		L"Resources/Shaders/PostEffect/CopyImage.VS.hlsl",
		L"Resources/Shaders/PostEffect/Bloom.PS.hlsl"
	);
}

// VignettePSO.cpp
void BloomPSO::CreateRootSignature() {
	D3D12_ROOT_PARAMETER rootParameters[2] = {};

	// 0: SRV
	D3D12_DESCRIPTOR_RANGE srvRange = {};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 1;
	srvRange.BaseShaderRegister = 0;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRange;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 1: CBV
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	rootParameters[1].Descriptor.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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
