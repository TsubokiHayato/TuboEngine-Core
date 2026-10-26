#include "Model.h"
#include "Matrix.h"
#include "Object3d.h"
#include "TextureManager.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

void TuboEngine::Model::Initialize(const std::string& directoryPath, const std::string& filename) {

	
	commandList = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();

#pragma region ModelData
	// モデル読み込み
	modelData = LoadModelFile(directoryPath, filename);
	// 頂点リソースを作る
	vertexResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	// 頂点バッファビューを作成する
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
#pragma endregion ModelData

#pragma region Material_Resource
	// マテリアル用のリソースを作る。今回はColor1つ分のサイズを用意する
	materialResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(Material));
	// マテリアルにデータを書き込む
	materialData = nullptr;
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 今回は赤を書き込んでみる
	materialData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	materialData->enableLighting = true;
	materialData->uvTransform = TuboEngine::Math::MakeIdentity4x4();
	materialData->shininess = 70.0f;
	materialData->environmentCoefficient = 1.0f;

#pragma endregion Material_Resource

	// テクスチャファイル名を抽出
	textureFileName_ = std::filesystem::path(modelData.material.textureFilePath).filename().string();

	// テクスチャを読み込む
	TuboEngine::TextureManager::GetInstance()->LoadTexture(textureFileName_);
	modelData.material.textureIndex = TuboEngine::TextureManager::GetInstance()->GetSrvIndex(textureFileName_);

	// インスタンスフラグ（0固定）を初期化して保持
	instancingFlagBuffer_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(int32_t));
	int32_t* commonData = nullptr;
	instancingFlagBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&commonData));
	*commonData = 0; // UseInstancing = false
	instancingFlagBuffer_->Unmap(0, nullptr);
}

void TuboEngine::Model::Draw(const std::string& textureOverride) {
	// 頂点バッファをセット
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	// マテリアルバッファをセット
	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

	// SRVのDescriptorTableの先頭を設定。2はRootParameter[2]である。（Object3d がテクスチャを差し替えていればそちら）
	commandList->SetGraphicsRootDescriptorTable(2, TuboEngine::TextureManager::GetInstance()->GetSrvHandleGPU(textureOverride.empty() ? textureFileName_ : textureOverride));
	
	// 共通データ（フラグ=0）をセット (RootParameter 10: b1 in VS)
	commandList->SetGraphicsRootConstantBufferView(10, instancingFlagBuffer_->GetGPUVirtualAddress());

	// 描画
	commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
}

void TuboEngine::Model::DrawSkinned(const D3D12_VERTEX_BUFFER_VIEW& influenceBufferView, const std::string& textureOverride) {
	// 頂点バッファ（スロット0）とインフルエンス（スロット1）をセット
	D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {vertexBufferView, influenceBufferView};
	commandList->IASetVertexBuffers(0, 2, vbvs);

	// マテリアルバッファをセット
	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

	// テクスチャをセット（Object3d がテクスチャを差し替えていればそちら）
	commandList->SetGraphicsRootDescriptorTable(2, TuboEngine::TextureManager::GetInstance()->GetSrvHandleGPU(textureOverride.empty() ? textureFileName_ : textureOverride));

	// 描画
	commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
}

void TuboEngine::Model::DrawInstanced(uint32_t instanceCount, D3D12_GPU_VIRTUAL_ADDRESS instanceBufferAddr, D3D12_GPU_VIRTUAL_ADDRESS commonBufferAddr) {
	// 頂点バッファをセット
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	// マテリアルバッファをセット
	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

	// テクスチャをセット
	commandList->SetGraphicsRootDescriptorTable(2, TuboEngine::TextureManager::GetInstance()->GetSrvHandleGPU(textureFileName_));

	// インスタンスデータをセット (RootParameter 9: t0 in VS)
	commandList->SetGraphicsRootShaderResourceView(9, instanceBufferAddr);

	// 共通データ（フラグ）をセット (RootParameter 10: b1 in VS)
	commandList->SetGraphicsRootConstantBufferView(10, commonBufferAddr);

	// 描画
	commandList->DrawInstanced(UINT(modelData.vertices.size()), instanceCount, 0, 0);
}

TuboEngine::MaterialData TuboEngine::Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filePath) {

	/*------------------------
	1 : 中で必要になる変数の宣言
	------------------------*/

	MaterialData materialData;
	std::string line;

	/*------------------------
	2 : ファイルを開く
	------------------------*/

	std::ifstream file(directoryPath + "/" + filePath);
	assert(file.is_open());

	/*---------------------------------------------
	3 : 実際にファイルを読み、MaterialDataを構築していく
	---------------------------------------------*/

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;

			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	/*------------------------
	4 : MaterialDataを返す
	------------------------*/

	return materialData;
}

TuboEngine::ModelData TuboEngine::Model::LoadModelFile(const std::string& directoryPath, const std::string& filename) {

	/*-------------
	1 : OBJファイル
	--------------*/
	ModelData modelData;            // 構築する
	std::vector<TuboEngine::Math::Vector4> positions; // 位置
	std::vector<TuboEngine::Math::Vector3> normals;   // 法線
	std::vector<TuboEngine::Math::Vector2> texcoords; // テクスチャ座標
	std::string line;               // ファイルから読んだ1行を格納するもの

	/*----------------------
	2 : OBJファイルを読み込む
	----------------------*/

	// Assimpを使ってOBJファイルを読み込む
	// 現状、objと
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes());

	modelData.rootNode = ReadNode(scene->mRootNode); // ノードを読み込む
	if (const aiNode* meshNode = FindMeshNode(scene->mRootNode)) {
		modelData.meshNodeName = meshNode->mName.C_Str();
	}

	/*-----------------------------
	3 : ファイルを読み、ModelDataを構築
	-------------------------------*/
	// ファイルを読み込む
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {

		/*--------------------------------
		            メッシュを解析
		--------------------------------*/

		aiMesh* mesh = scene->mMeshes[meshIndex];
		// メッシュの確認
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		// メッシュの頂点番号 → 展開後の modelData.vertices の番号（スキニングのウェイト対応付けに使う）
		std::vector<std::vector<uint32_t>> expandedIndices(mesh->mNumVertices);

		/*--------------------------------
		            faceを解析
		---------------------------------*/

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形のみ対応

			/*--------------------------------
			        vertexを解析
			--------------------------------*/

			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				uint32_t vetexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vetexIndex];
				aiVector3D& normal = mesh->mNormals[vetexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vetexIndex];
				VertexData vertex;
				vertex.position = {position.x, position.y, position.z, 1.0f};
				vertex.normal = {normal.x, normal.y, normal.z};
				vertex.texcoord = {texcoord.x, texcoord.y};
				// aiProcess_MakeLeftHandedはz*=-1で、右手->左手に変換するので手動で対処
				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;
				expandedIndices[vetexIndex].push_back(uint32_t(modelData.vertices.size()));
				modelData.vertices.push_back(vertex);
			}

			/*--------------------------------
			        materialを解析
			--------------------------------*/

			for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
				aiMaterial* material = scene->mMaterials[materialIndex];
				if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
					aiString texturePath;
					material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
					modelData.material.textureFilePath = directoryPath + "/" + texturePath.C_Str();
				}
			}
		}

		/*--------------------------------
		        boneを解析（スキニング）
		--------------------------------*/

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

			// BindPoseMatrix に戻し、右手→左手に変換してから再び逆行列にする
			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix;
			bindPoseMatrixAssimp.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
			TuboEngine::Math::Matrix4x4 bindPoseMatrix = TuboEngine::Math::MakeAffineMatrix(
			    {scale.x, scale.y, scale.z}, TuboEngine::Math::Quaternion(rotate.x, -rotate.y, -rotate.z, rotate.w), {-translate.x, translate.y, translate.z});
			jointWeightData.inverseBindPoseMatrix = TuboEngine::Math::Inverse(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				const aiVertexWeight& weight = bone->mWeights[weightIndex];
				for (uint32_t expandedIndex : expandedIndices[weight.mVertexId]) {
					jointWeightData.vertexWeights.push_back({weight.mWeight, expandedIndex});
				}
			}
		}
	}

	return modelData;
}

TuboEngine::Node TuboEngine::Model::ReadNode(aiNode* node) {

	Node result;
	// ノードのローカル変換を分解し、右手→左手に変換する（頂点の x 反転と揃える）
	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);
	result.transform.scale = {scale.x, scale.y, scale.z};
	result.transform.rotate = TuboEngine::Math::Quaternion(rotate.x, -rotate.y, -rotate.z, rotate.w);
	result.transform.translate = {-translate.x, translate.y, translate.z};
	result.localMatrix = TuboEngine::Math::MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name = node->mName.C_Str();          // ノード名を取得
	result.children.resize(node->mNumChildren); // 子ノードの数を取得
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		// 子ノードを再帰的に読み込む
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);

	}

	return result;
}

const aiNode* TuboEngine::Model::FindMeshNode(const aiNode* node) {
	if (node->mNumMeshes > 0) {
		return node;
	}
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		if (const aiNode* found = FindMeshNode(node->mChildren[childIndex])) {
			return found;
		}
	}
	return nullptr;
}
