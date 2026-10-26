#pragma once



#include "Material.h"
#include "ModelData.h"
#include "Node.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "VertexData.h"
#include"DirectXCommon.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
namespace TuboEngine {
/// <summary>
/// 3Dモデル。頂点データ・マテリアル・テクスチャを保持し描画する。
/// </summary>
class Model {
public:
	//------------------------------------
	// メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="modelCommon">モデル共通部分</param>
	void Initialize(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw(const std::string& textureOverride = "");

	/// <summary>
	/// スキニング描画処理（スロット1にインフルエンスの頂点バッファを追加でセットする）
	/// </summary>
	/// <param name="influenceBufferView">SkinCluster のインフルエンス頂点バッファビュー</param>
	void DrawSkinned(const D3D12_VERTEX_BUFFER_VIEW& influenceBufferView, const std::string& textureOverride = "");

	/// <summary>
	/// インスタンス描画処理
	/// </summary>
	/// <param name="instanceCount">インスタンス数</param>
	/// <param name="instanceBufferAddr">インスタンスデータのGPUアドレス</param>
	/// <param name="commonBufferAddr">共通データのGPUアドレス</param>
	void DrawInstanced(uint32_t instanceCount, D3D12_GPU_VIRTUAL_ADDRESS instanceBufferAddr, D3D12_GPU_VIRTUAL_ADDRESS commonBufferAddr);

	/// <summary>
	/// マテリアルテンプレートファイルを読み込む
	/// </summary>
	/// <param name="directoryPath">ディレクトリパス</param>
	/// <param name="filePath">ファイルパス</param>
	/// <returns>マテリアルデータ</returns>
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filePath);

	/// <summary>
	/// OBJファイルを読み込む
	/// </summary>
	/// <param name="directoryPath">ディレクトリパス</param>
	/// <param name="filename">ファイル名</param>
	/// <returns>モデルデータ</returns>
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// ノードを読み込む
	/// </summary>
	/// <param name="node">ノード</param>
	/// <returns></returns>
	static Node ReadNode(aiNode* node);

	/// <summary>
	/// メッシュを持つ最初のノードを探す
	/// </summary>
	static const aiNode* FindMeshNode(const aiNode* node);

public:
	//-----------------------------------------------------------
	// Setter&Getter

	// モデルのカラー
	Vector4 GetModelColor() { return materialData->color; }
	void SetModelColor(const Vector4& color) { materialData->color = color; }

	// 光沢度
	float GetModelShininess() { return materialData->shininess; }
	void SetModelShininess(float shininess) { materialData->shininess = shininess; }

	// 環境マップ寄与度
	float GetModelEnvironmentCoefficient() { return materialData->environmentCoefficient; }
	void SetModelEnvironmentCoefficient(float coefficient) { materialData->environmentCoefficient = coefficient; }

	// rootNodeLocalMatrix
	Matrix4x4 GetRootNodeLocalMatrix() { return modelData.rootNode.localMatrix; }
	void SetRootNodeLocalMatrix(const Matrix4x4& matrix) { modelData.rootNode.localMatrix = matrix; }

	// モデルのテクスチャ名（Resources/Textures からのパス）
	const std::string& GetTextureName() const { return textureFileName_; }

	// モデルデータ（アニメーション・スキニング用）
	const ModelData& GetModelData() const { return modelData; }
	// スキニング情報を持っているか
	bool IsSkinned() const { return !modelData.skinClusterData.empty(); }

private:
	// モデルデータ
	ModelData modelData;

	// 頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	// 頂点バッファ内リソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	// 頂点バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	// マテリアルのバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	// マテリアルのバッファリソース内のデータを指すポインタ
	Material* materialData = nullptr;

	// コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	std::string textureFileName_;

	// インスタンスフラグ保持用
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingFlagBuffer_;
};
} // namespace TuboEngine