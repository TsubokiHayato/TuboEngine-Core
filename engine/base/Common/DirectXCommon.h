#pragma once

#include<d3d12.h>
#include<dxgi1_6.h>
#include<wrl.h>
#include <format>
#include"Logger.h"
#include"StringUtility.h"
#include"array"
#include"WinApp.h"
#include <dxcapi.h>
#include "externals/DirectXTex/d3dx12.h"
#include "externals/DirectXTex/DirectXTex.h"
#include<vector>
#include<chrono>
#include<Vector4.h>
#include<OffScreenRenderingPSO.h>

namespace TuboEngine {
class DirectXCommon {

private:
	// シングルトン用
	DirectXCommon() = default;
	~DirectXCommon() = default;
	DirectXCommon(const DirectXCommon&) = delete;
	DirectXCommon& operator=(const DirectXCommon&) = delete;
	DirectXCommon(DirectXCommon&&) = delete;
	DirectXCommon& operator=(DirectXCommon&&) = delete;

public:
	// インスタンス取得
	static DirectXCommon* GetInstance() {
		// 静的ローカル変数によるシングルトン（new/delete 不要）
		static DirectXCommon instance;
		return &instance;
	}

public:
	// 初期化
	void Initialize();

	void Finalize();

	// デバイスの初期化
	void Device_Initialize();
	// コマンド関連の初期化
	void Command_Initialize();
	// スワップチェーンの生成
	void SwapChain_Create();
	// 深度バッファの生成
	void DepthBuffer_Create(int32_t width, int32_t height);
	// 各種ディスクリプタヒープの生成
	void DescriptorHeap_Create();
	// レンダーターゲットビューの初期化
	void RTV_Initialize();
	// 深度ステンシルビューの初期化
	void DSV_Initialize();

	// フェンスの生成
	void Fence_Create();
	// ビューポート矩形の初期化
	void Viewport_Initialize();
	// シザリング矩形の初期化
	void Scissor_Initialize();
	// DXCコンパイラの生成
	void dxcCompiler_Create();

	// 描画前処理 Begin
	void PreDraw();
	// 描画後処理 End
	void PostDraw();

	// コマンドリストのリセット
	void CommandExecution();

	// DescriptorHeapの作成関数
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	//
	// ∧__∧
	//(｀Д´ ）
	//(っ▄︻▇〓┳═getter
	///    )
	//(/ ￣∪
	//
	// Device
	Microsoft::WRL::ComPtr<IDXGIFactory7> GetDxgiFactory() const { return dxgiFactory; }

	Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const { return device; }

	// Command
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const { return commandAllocator; }
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return commandList; }
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return commandQueue; }

	// swapChain
	Microsoft::WRL::ComPtr<IDXGISwapChain4> GetSwapChain() const { return swapChain; }
	DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc() const { return swapChainDesc; }

	// CPUのDescriptorHandleを取得する関数
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	// GPUのDescriptorHandleを取得する関数
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	// SRVの指定番号のCPUディスクリプタハンドルを取得する
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	// SRVの指定番号のGPUディスクリプタハンドルを取得する
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	// RTVの指定番号のCPUディスクリプタハンドルを取得する
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index);
	// RTVの指定番号のGPUディスクリプタハンドルを取得する
	D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUDescriptorHandle(uint32_t index);

	// DSVの指定番号のCPUディスクリプタハンドルを取得する
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle(uint32_t index);
	// DSVの指定番号のGPUディスクリプタハンドルを取得する
	D3D12_GPU_DESCRIPTOR_HANDLE GetDSVGPUDescriptorHandle(uint32_t index);

	// ディスクリプタヒープの取得

	// DescriptorSizeの取得
	uint32_t GetDescriptorSizeSRV() { return descriptorSizeSRV; }
	// DescriptorSizeの取得
	uint32_t GetDescriptorSizeRTV() { return descriptorSizeRTV; }
	// DescriptorSizeの取得
	uint32_t GetDescriptorSizeDSV() { return descriptorSizeDSV; }

	// RTVディスクイリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetRtvDescriptorHeap() { return rtvDescriptorHeap; }

	// SRVディスクイリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSrvDescriptorHeap() { return srvDescriptorHeap; }

	// DSVディスクイリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDsvDescriptorHeap() { return dsvDescriptorHeap; }

	D3D12_RENDER_TARGET_VIEW_DESC GetRtvDesc() { return rtvDesc; }
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[3] = {};

	Microsoft::WRL::ComPtr<ID3D12Fence> GetFence() { return fence; }
	uint64_t GetFenceValue() { return fenceValue; }
	HANDLE GetFenceEvent() { return fenceEvent; }

	// ビューポート
	D3D12_VIEWPORT GetViewport() { return viewport; }
	// シザー矩形
	D3D12_RECT GetScissorRect() { return scissorRect; }

	// dxcCompilerを初期化
	IDxcUtils* GetDxcUtils() { return dxcUtils; }
	IDxcCompiler3* GetDxcCompiler() { return dxcCompiler; }
	IDxcIncludeHandler* GetIncludeHandler() { return includeHandler; }

	size_t GetBackBufferCount() const { return swapChainDesc.BufferCount; }

	// viewportの取得
	D3D12_VIEWPORT GetViewport() const { return viewport; }
	// scissorRectの取得
	D3D12_RECT GetScissorRect() const { return scissorRect; }

	// シェーダーのコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);

	// depthStencilResourceの取得
	Microsoft::WRL::ComPtr<ID3D12Resource> GetDepthStencliResouece() { return depthStencilResource; }

	///< summary>
	/// バッファリソースの生成
	///</summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	///< summary>
	/// テクスチャリソースの生成
	///</summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	///< summary>
	/// テクスチャデータの転送
	///</summary>
	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);

	///< summary>
	/// テクスチャファイルの読み込み
	///</summary>
	/// <param name="filePath>テクスチャファイルのパス</param>
	/// <returns>画像イメージデータ</returns>
	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	// HRESULTはWindow系のエラーコードであり、
	// 関数が成功したかどうかSUCCEEDEDマクロで判断出来る
	HRESULT hr = 0;

private:
	// FPS固定初期化
	void InitializeFixFPS();
	// FPS固定更新
	void UpdateFixFPS();

	// DXGIファクトリーの設置
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;

	// Command
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;

	// swapChain
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	uint32_t descriptorSizeSRV = 0;
	uint32_t descriptorSizeRTV = 0;
	uint32_t descriptorSizeDSV = 0;

	// RTVディスクイリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;

	// SRVディスクイリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;

	// DSVディスクイリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	HANDLE fenceEvent = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

	// ビューポート
	D3D12_VIEWPORT viewport{};
	// シザー矩形
	D3D12_RECT scissorRect{};

	// dxcCompilerを初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	IDxcIncludeHandler* includeHandler = nullptr;

	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};

	UINT backBufferIndex = {};

	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers; // Add this line to define backBuffers
};

} // namespace TuboEngine