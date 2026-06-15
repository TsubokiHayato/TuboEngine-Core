#include "ModelManager.h"
ModelManager* ModelManager::instance = nullptr;

ModelManager* ModelManager::GetInstance()
{
	if (instance == nullptr) {
		instance = new ModelManager;
	}
	return instance;
}

void ModelManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void ModelManager::initialize()
{

}

void ModelManager::LoadModel(const std::string& filePath)
{
	//読み込み済みモデルを検索
	if (models.contains(filePath)) {
		//読み込み済みなら早期リターン
		return;
	}
	//モデルの生成とファイルの読み込み、初期化
	std::unique_ptr<TuboEngine::Model> model = std::make_unique<TuboEngine::Model>();
	model->Initialize("Resources/Models", filePath);
	
	//モデルをmapコンテナに格納する
	models.insert(std::make_pair(filePath, std::move(model)));

}

TuboEngine::Model* ModelManager::FindModel(const std::string& filePath) {
	//モデルの検索
	if (models.contains(filePath)) {
		return models.at(filePath).get();
	}
	return nullptr;
}

