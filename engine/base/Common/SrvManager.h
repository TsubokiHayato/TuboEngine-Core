#pragma once
#include "DirectXCommon.h"
namespace TuboEngine {
class SrvManager {
public:
	/// <summary>
	/// シングルトンインスタンス取得
	/// </summary>
	static SrvManager* GetInstance() {

		if (!instance) {
			instance = new SrvManager();
		}
		return instance;
	}

private:
	// コンストラクタ・デストラクタ・コピー禁止
	static SrvManager* instance;
	SrvManager() = default;
	~SrvManager() = default;
	SrvManager(const SrvManager&) = delete;
	SrvManager& operator=(const SrvManager&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon">DirectX共通部分</param>
	void Initialize();

	/// <summary>
	/// SRVの割り当て
	/// </summary>
	/// <returns></returns>
	uint32_t Allocate();

	/// <summary>
	/// ディスクリプタヒープのCPUハンドルを取得
	/// </summary>
	/// <param name="index">ディスクリプタヒープのインデックス</param>
	/// <param name="pResource">リソース</param>
	/// <param name="format">フォーマット</param>
	/// <param name="MipLevels">ミップマップレベル</param>
	void CreateSRVforTexture2D(uint32_t index, Microsoft::WRL::ComPtr<ID3D12Resource> pResource, DXGI_FORMAT format, UINT MipLevels);

	/// <summary>
	/// ディスクリプタヒープのCPUハンドルを取得
	/// </summary>
	/// <param name="index">ディスクリプタヒープのインデックス</param>
	/// <param name="pResource">リソース</param>
	/// <param name="numElements">要素数</param>
	/// <param name="strideInBytes">バイト数</param>
	void CreateSRVForStructuredBuffer(uint32_t srvIndex, Microsoft::WRL::ComPtr<ID3D12Resource> pResource, UINT enelemtQuantity, UINT structureByteStride);

	/// @brief UAV (Unordered Access View) 作成 — Compute Shader が RW アクセスするバッファ用
	void CreateUAVForStructuredBuffer(uint32_t index, ID3D12Resource* pResource,
	                                   UINT elementCount, UINT structureByteStride);
	/// <summary>
	/// 描画前処理
	/// </summary>
	void PreDraw();

	void Finalize();

	//-------------------Getter & Setter-------------------//
	/// <summary>
	/// ディスクリプタヒープのCPUハンドルを取得
	/// </summary>
	/// <param name="index">ディスクリプタヒープのインデックス</param>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);

	ID3D12DescriptorHeap* GetDescriptorHeap() const { return descriptorHeap.Get(); }
	/// <summary>
	/// ディスクリプタヒープのGPUハンドルを取得
	/// </summary>
	/// <param name="index">ディスクリプタヒープのインデックス</param>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

	void SetGraphicsRootDescriptorTable(uint32_t rootParameterIndex, uint32_t srvIndex) {
		TuboEngine::DirectXCommon::GetInstance()->GetCommandList()->SetGraphicsRootDescriptorTable(rootParameterIndex, GetGPUDescriptorHandle(srvIndex));
	}

	// 最大SRV数(最大テクスチャ枚数)
	static const uint32_t kMaxSRVCount;

private:
	// SRV用のディスクリプタサイズ
	uint32_t descriptorSize = 0;
	// SRVヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	// 次に使用するSRVのインデックス
	uint32_t useIndex = 0;
};

} // namespace TuboEngine