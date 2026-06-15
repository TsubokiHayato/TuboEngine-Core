#include "BlendPSO.h"

void BlendPSO::Initialize(BlendMode blendMode)
{
	blendMode_ = blendMode;
	device = TuboEngine::DirectXCommon::GetInstance()->GetDevice();
	commandList = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();

	// Create root signature and compile shaders once
	CreateRootSignature();

	// Defer creating pipeline state until needed
	CreateOrUpdatePipelineState();
}

void BlendPSO::DrawSettingsCommon()
{
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->SetPipelineState(graphicsPipeLineState.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void BlendPSO::CreateRootSignature()
{
	if (rootSignatureCreated_) return;
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// DescriptorRange
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// t1 (gCubeTexture)
	descriptorRange[1].BaseShaderRegister = 1;
	descriptorRange[1].NumDescriptors = 1;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameter
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
	rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;

	for (int i = 0; i < 5; ++i) {
		rootParameters[4 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[4 + i].Descriptor.ShaderRegister = 1 + i;
	}

	rootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV; // StructuredBuffer
	rootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[9].Descriptor.ShaderRegister = 0; // t0

	rootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // ConstantBuffer
	rootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[10].Descriptor.ShaderRegister = 1; // b1

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	// Sampler
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	TuboEngine::DirectXCommon::GetInstance()->hr = D3D12SerializeRootSignature(
		&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	if (FAILED(TuboEngine::DirectXCommon::GetInstance()->hr)) {
		TuboEngine::Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	TuboEngine::DirectXCommon::GetInstance()->hr = device->CreateRootSignature(
		0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(TuboEngine::DirectXCommon::GetInstance()->hr));

	rootSignatureCreated_ = true;
}

void BlendPSO::SetupBlendState()
{
	blendDesc = {};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	switch (blendMode_) {
	case kBlendModeNone:
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		break;

	case kBlendModeNormal:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		break;

	case kBlendModeAdd:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		break;

	case kBlendModeSubtract:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		break;

	case kBlendModeMultily:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		break;

	case kBlendModeScreen:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		break;

	default:
		// fall back
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		break;
	}
}

void BlendPSO::CreateGraphicPipeline()
{
	// Deprecated: now handled by CreateOrUpdatePipelineState
	CreateOrUpdatePipelineState();
}

void BlendPSO::CreateOrUpdatePipelineState()
{
	// Ensure root signature exists
	if (!rootSignatureCreated_) CreateRootSignature();

	// Compile shaders once
	if (!shadersCompiled_) {
		vertexShaderBlob = TuboEngine::DirectXCommon::GetInstance()->CompileShader(
			L"Resources/Shaders/Object3d.VS.hlsl", L"vs_6_0");
		assert(vertexShaderBlob != nullptr);

		pixelShaderBlob = TuboEngine::DirectXCommon::GetInstance()->CompileShader(
			L"Resources/Shaders/Object3d.PS.hlsl", L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		shadersCompiled_ = true;
	}

	// InputLayout
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendState (only differs per mode)
	SetupBlendState();

	// RasterizerState
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// DepthStencil
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSO
	graphicPipelineStateDesc = {};
	graphicPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	graphicPipelineStateDesc.BlendState = blendDesc;
	graphicPipelineStateDesc.RasterizerState = rasterizerDesc;

	graphicPipelineStateDesc.NumRenderTargets = 1;
	graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicPipelineStateDesc.SampleDesc.Count = 1;
	graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// If pipeline already exists but blend state changed, recreate PSO
	if (pipelineStateCreated_)
	{
		// Compare existing blend mode; if unchanged, do nothing
		// For simplicity, recreate if different blendMode_ (caller should change blendMode_ before calling)
		// Destroy existing PSO before creating new one
		graphicsPipeLineState.Reset();
		pipelineStateCreated_ = false;
	}

	 TuboEngine::DirectXCommon::GetInstance()->hr = device->CreateGraphicsPipelineState(
		&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipeLineState));
	assert(SUCCEEDED(TuboEngine::DirectXCommon::GetInstance()->hr));

	pipelineStateCreated_ = true;
}

void BlendPSO::SetBlendMode(BlendMode newMode)
{
	if (blendMode_ == newMode) return;
	blendMode_ = newMode;
	// Recreate pipeline state with new blend settings
	CreateOrUpdatePipelineState();
}
