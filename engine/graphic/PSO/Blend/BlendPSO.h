#pragma once

#include "DirectXCommon.h"
#include "BlendMode.h"

// ブレンドモードをパラメータ化したPSO
// これまで Add/Normal/Subtract/Multiply/Screen ごとに分けていたクラスを集約
class BlendPSO
{
public:
	/// <summary>
	/// 初期化（指定ブレンドモードでPSOを作成）
	/// </summary>
	void Initialize(BlendMode blendMode);

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawSettingsCommon();

	BlendMode GetBlendMode() const { return blendMode_; }

	/// <summary>
	/// ブレンドモードを切り替える（既に同じモードなら何もしない）
	/// </summary>
	void SetBlendMode(BlendMode newMode);

private:
	void CreateRootSignature();
	void CreateGraphicPipeline();
	void SetupBlendState();

	// 内部 helper: 実際のパイプラインステートのみを（必要に応じて）作成
	void CreateOrUpdatePipelineState();

private:
	// RootSignature
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};
	D3D12_ROOT_PARAMETER rootParameters[11] = {};
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};

	// Blend/Rasterizer/Depth
	D3D12_BLEND_DESC blendDesc{};
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// Shader
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob;

	// PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	BlendMode blendMode_ = kBlendModeNormal;

	// 初期化済みフラグ
	bool rootSignatureCreated_ = false;
	bool shadersCompiled_ = false;
	bool pipelineStateCreated_ = false;
};
