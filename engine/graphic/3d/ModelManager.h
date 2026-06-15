#pragma once
#include <map>
#include <memory>
#include <string>
#include "Model.h"


class ModelManager
{
private:
	static ModelManager* instance;//シングルトンインスタンス

	ModelManager() = default;//コンストラクタの隠蔽
	~ModelManager() = default;//デストラクタの隠蔽
	ModelManager(ModelManager&) = delete;//コピーコンストラクタの封印
	ModelManager& operator=(ModelManager&) = delete;//代入演算子の封印


	std::map<std::string, std::unique_ptr<TuboEngine::Model>> models;
	
public:

	//シングルトンインスタンスの取得
	/// <summary>
	///シングルトンインスタンスの取得
	/// </summary>
	static ModelManager* GetInstance();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon">DirectX基盤</param>
	void initialize();

	/// <summary>
	/// モデルの読み込み
	/// </summary>
	/// <param name="filePath">モデルファイルのパス</param>
	void LoadModel(const std::string& filePath);

	/// <summary>
	/// モデルの検索
	/// </summary>
	/// <param name="filePath">モデルファイルのパス</param>
	/// <returns>モデル</returns>
	TuboEngine::Model* FindModel(const std::string& filePath);

};
