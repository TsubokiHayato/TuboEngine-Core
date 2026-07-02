#pragma once
#include<string>
#include"externals/DirectXTex/DirectXTex.h"
#include"DirectXcommon.h"
#include"SrvManager.h"
#include<vector>
#include<unordered_map>

namespace TuboEngine {

/// <summary>
/// テクスチャの読み込み・SRV生成・キャッシュを管理するクラス（シングルトン）。
/// </summary>
class TextureManager
{
private:

	/// <summary>
	/// コンストラクタ。
	/// </summary>
	TextureManager() = default;
	/// <summary>
	/// デストラクタ。
	/// </summary>
	~TextureManager() = default;
	/// <summary>
	/// コピー禁止。
	/// </summary>
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

	/// <summary>
	/// テクスチャ1枚分のリソースとSRV情報。
	/// </summary>
	struct TextureData
	{
		std::string filePath;//画像のファイルパス
		DirectX::TexMetadata metadata;//画像の高さなどの情報
		Microsoft::WRL::ComPtr<ID3D12Resource>resource;//テクスチャリソース
		uint32_t srvIndex;//SRVのインデックス
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;//SRV作成時に必要なCPUハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;//描画コマンドに必要なGPUハンドル


	};




	//テクスチャデータ
	std::unordered_map<std::string, TextureData> textureDatas;

public:
	//シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	//終了
	void Finalize();
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// テクスチャの読み込み。
	/// </summary>
	void LoadTexture(const std::string& filePath);


	/// <summary>
	/// メタデータを取得
	/// </summary>
	/// <param name="textureIndex"></param>
	/// <returns></returns>
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

	/// <summary>
	///Srvインデックスの取得
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	uint32_t GetSrvIndex(const std::string& filePath);

	/// <summary>
	/// SrvHandleGPU を取得する。
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	//SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;


private:
	
	std::string directoryPath_;//ディレクトリパス
	std::string filePath_;//ファイルパス

	//ディレクトリパスとファイルパスを結合する変数
	std::string fullPath_;

};

} // namespace TuboEngine

