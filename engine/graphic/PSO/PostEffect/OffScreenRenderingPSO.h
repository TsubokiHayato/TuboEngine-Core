#pragma once

#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <dxcapi.h>


// 前方宣言
class DirectXCommon;

class OffScreenRenderingPSO
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon">DirectX共通部分</param>
	void Initialize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawSettingsCommon();

private:
	/*---------------------------------------------------
			関数
	---------------------------------------------------*/

	/// <summary>
	/// ルートシグネイチャの作成
	/// </summary>
	void CreateRootSignature();

	/// <summary>
	/// グラフィックスパイプラインの作成
	/// </summary>
	void CreateGraphicPipeline();

	/*-----------------
		rootSignature
	-----------------*/


	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	//DescriptorRange作成
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	//RootParameter作成。
	D3D12_ROOT_PARAMETER rootParameters[1] = {};
	//Sampler作成
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr <ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr <ID3DBlob> errorBlob = nullptr;
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	/*------------
	  InputLayOut
	------------*/
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	/*------------
	  BlendState
	------------*/

	D3D12_BLEND_DESC blendDesc{};
	/*------------------
	  RasterizerState
	------------------*/

	//RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	/*-------------------
	  Vertex&Pixel_Shader
	-------------------*/

	//Shaderをコンパイルする
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob;

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob;
	/*---------------
	DepthStencilDescの設定
	-------------------*/

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	/*------------------
	 　 PSOを生成する
	------------------*/
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipeLineState = nullptr;

	Microsoft::WRL::ComPtr <ID3D12Device> device;
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> commandList;


};

