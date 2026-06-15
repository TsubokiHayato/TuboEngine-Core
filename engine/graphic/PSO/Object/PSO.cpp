#include "PSO.h"

void PSO::Initialize() {

	device = TuboEngine::DirectXCommon::GetInstance()->GetDevice();
	commandList = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();


	//グラフィックスパイプラインの作成
	CreateGraphicPipeline();

}

void PSO::DrawSettingsCommon() {
	/*ルートシグネイチャをセットするコマンド*/
	 //RootSignatureを設定。

	commandList->SetGraphicsRootSignature(rootSignature.Get());


	/*グラフィクスパイプラインステートをセットするコマンド*/

	commandList->SetPipelineState(graphicsPipeLineState.Get());//PSOを設定


	/*プリミティブトポロジーをセットするコマンド*/
	//形状設定
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}
void PSO::CreateRootSignature() {
	// RootSignature作成
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// DescriptorRange作成
	descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRange[0].NumDescriptors = 1; // 数は1
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVをつかう
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

	// 追加
	descriptorRange[1].BaseShaderRegister = 1; // t1 (gCubeTexture)
	descriptorRange[1].NumDescriptors = 1;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	// RootParameter作成
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderでつかう
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderでつかう
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DESCRIPTOR_TABLEを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderでつかう
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

	// 例: t1用
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
	rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;


	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderでつかう
	rootParameters[4].Descriptor.ShaderRegister = 1; // レジスタ番号1とバインド

	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderでつかう
	rootParameters[5].Descriptor.ShaderRegister = 2; // レジスタ番号2とバインド

	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderでつかう
	rootParameters[6].Descriptor.ShaderRegister = 3; // レジスタ番号3とバインド

	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderでつかう
	rootParameters[7].Descriptor.ShaderRegister = 4; // レジスタ番号4とバインド

	rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderでつかう
	rootParameters[8].Descriptor.ShaderRegister = 5; // レジスタ番号5とバインド

	rootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV; // StructuredBuffer
	rootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[9].Descriptor.ShaderRegister = 0; // t0

	rootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // ConstantBuffer
	rootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[10].Descriptor.ShaderRegister = 1; // b1

	descriptionRootSignature.pParameters = rootParameters; // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ

	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイアニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipMapをつかう
	staticSamplers[0].ShaderRegister = 0; // レジスタ番号0をつかう
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderでつかう
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	TuboEngine::DirectXCommon::GetInstance()->hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	if (FAILED(TuboEngine::DirectXCommon::GetInstance()->hr)) {
		TuboEngine::Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に作成
	TuboEngine::DirectXCommon::GetInstance()->hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(TuboEngine::DirectXCommon::GetInstance()->hr));
}

void PSO::CreateGraphicPipeline() {
	//ルートシグネイチャ」の作成
	CreateRootSignature();



	/*------------
	  InputLayOut
	------------*/

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

	/*------------
	  BlendState
	------------*/


	//すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;
   // デフォルトは不透明描画（ブレンド無効）
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;



	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;


	/*------------------
	  RasterizerState
	------------------*/

	//裏面表示
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	/*-------------------
	  Vertex&Pixel_Shader
	-------------------*/

	//Shaderをコンパイルする
	vertexShaderBlob = TuboEngine::DirectXCommon::GetInstance()->CompileShader(L"Resources/Shaders/Object3d.VS.hlsl",
		L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	pixelShaderBlob = TuboEngine::DirectXCommon::GetInstance()->CompileShader(L"Resources/Shaders/Object3d.PS.hlsl",
		L"ps_6_0");
	assert(pixelShaderBlob != nullptr);


	/*---------------
	DepthStencilDescの設定
	-------------------*/

	//Depthの機能を有効化
	depthStencilDesc.DepthEnable = true;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数はLessEqual
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;


	/*------------------
	 　 PSOを生成する
	------------------*/

	graphicPipelineStateDesc.pRootSignature = rootSignature.Get();//RootSignature
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc;//InputLayOut

	graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };//vertexShader

	graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };//pixcelShader

	graphicPipelineStateDesc.BlendState = blendDesc;//BlendState
	graphicPipelineStateDesc.RasterizerState = rasterizerDesc;//RasterizerState

	//書き込むRTVの情報
	graphicPipelineStateDesc.NumRenderTargets = 1;
	graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//利用するトポロジ(形状)のタイプ。三角形
	graphicPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//どのように画面に書き込むのかの設定
	graphicPipelineStateDesc.SampleDesc.Count = 1;
	graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;


	//DepthStencil
	graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//実際に生成
	TuboEngine::DirectXCommon::GetInstance()->hr = device->CreateGraphicsPipelineState(&graphicPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipeLineState));
	assert(SUCCEEDED(TuboEngine::DirectXCommon::GetInstance()->hr));


}
