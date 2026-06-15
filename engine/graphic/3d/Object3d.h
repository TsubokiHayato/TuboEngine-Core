#pragma once
#include"DirectXcommon.h"

#include"VertexData.h"
#include"TransformationMatrix.h"
#include"Transform.h"
#include"CameraForGPU.h"
#include"SkyBox.h"
#include"Model.h"
//前方宣言
class Object3dCommon;
class ModelCommon;
class Camera;

//平行光源
struct DirectionalLight {
	//色
	TuboEngine::Math::Vector4 color;
	//方向
	TuboEngine::Math::Vector3 direction;
	//強度
	float intensity;
};

//PointLight
struct PointLight
{
	//色
	TuboEngine::Math::Vector4 color;
	//位置
	TuboEngine::Math::Vector3 position;
	//輝度
	float intensity;
};

struct SpotLight
{
	//色
	TuboEngine::Math::Vector4 color;
	//位置
	TuboEngine::Math::Vector3 position;
	//輝度
	float intensity;
	//方向
	TuboEngine::Math::Vector3 direction;
	//ライトの届く最大距離
	float distance;
	//減衰率
	float decay;
	//スポットライトの余弦
	float cosAngle;
	//
	float padding[2];
};

struct LightType {

	//0 : 平行光源
	//1 : Phong反射モデル
	//2 : Blinn-Phong反射モデル
	//3 : PointLight
	//4 : SpotLight
	int type;

};

namespace TuboEngine {

// Object3d.cpp内で保持している共有ライト用GPUリソースを明示解放する（終了時リークチェック対策）
void SharedLightResourcesRelease();

class Object3d {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="object3dCommon"></param>
	void Initialize(std::string modelFileNamePath);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// ImGuiで全機能をまとめて操作・確認する関数
	/// </summary>
	void DrawImGui(const char* windowName);

public:
	// Setter
	void SetScale(const TuboEngine::Math::Vector3& scale) { transform.scale = scale; }
	void SetRotation(const TuboEngine::Math::Vector3& rotation) { transform.rotate = rotation; }
	void SetPosition(const TuboEngine::Math::Vector3& position) { transform.translate = position; }

	///-------------------------------------------------------------------------------------------------
	/// Light
	// 平行光源
	void SetLightColor(const TuboEngine::Math::Vector4& color) { directionalLightData->color = color; }
	void SetLightDirection(const TuboEngine::Math::Vector3& direction) { directionalLightData->direction = direction; }
	void SetLightIntensity(float intensity) { directionalLightData->intensity = intensity; }
	void SetLightShininess(float shininess);
	// ポイントライト
	void SetPointLightPosition(const TuboEngine::Math::Vector3& position) { pointLightData->position = position; }
	void SetPointLightColor(const TuboEngine::Math::Vector4& color) { pointLightData->color = color; }
	void SetPointLightIntensity(float intensity) { pointLightData->intensity = intensity; }
	// スポットライト
	void SetSpotLightColor(const TuboEngine::Math::Vector4& color) { spotLightData->color = color; }
	void SetSpotLightPosition(const TuboEngine::Math::Vector3& position) { spotLightData->position = position; }
	void SetSpotLightDirection(const TuboEngine::Math::Vector3& direction) { spotLightData->direction = direction; }
	void SetSpotLightIntensity(float intensity) { spotLightData->intensity = intensity; }
	void SetSpotLightDistance(float distance) { spotLightData->distance = distance; }
	void SetSpotLightDecay(float decay) { spotLightData->decay = decay; }
	void SetSpotLightCosAngle(float cosAngle) { spotLightData->cosAngle = cosAngle; }

   void SetLightType(int type) {
		if (type < 0 || type > 5) {
			type = 0;
		}
		lightType_ = type;
	}

	void SetModel(TuboEngine::Model* model) {
		assert(model);
		this->model_ = model;
	}
	void SetModel(const std::string& filePath);
	void SetCamera(TuboEngine::Camera* camera) { this->camera = camera; }
	TuboEngine::Camera* GetCamera() const { return camera; }

	void SetModelColor(const TuboEngine::Math::Vector4& color);

	// Getter
	TuboEngine::Math::Vector3 GetScale() const { return transform.scale; }
	TuboEngine::Math::Vector3 GetRotation() const { return transform.rotate; }
	TuboEngine::Math::Vector3 GetPosition() const { return transform.translate; }
	// モデルの色
	Vector4 GetModelColor();

	// Getter for Batching
	TuboEngine::Model* GetModel() const { return model_; }
	TransformationMatrix* GetTransformationMatrixData() const { return transformMatrixData; }
	
	ID3D12Resource* GetDirectionalLightResource() const { return directionalLightResource.Get(); }
	ID3D12Resource* GetPointLightResource() const { return pointLightResource.Get(); }
	ID3D12Resource* GetSpotLightResource() const { return spotLightResource.Get(); }
	ID3D12Resource* GetCameraForGPUResource() const { return cameraForGPUResource.Get(); }
	ID3D12Resource* GetLightTypeResource() const { return lightTypeResource.Get(); }
	std::string GetCubeMapFilePath() const { return cubeMapFilePath_; }

	//-------------------------------------------------------------------------------------------------
	// Material

	///-------------------------------------------------------------------------------------------------
	/// Light
	// 平行光源
	TuboEngine::Math::Vector4 GetLightColor() { return directionalLightData->color; }
	TuboEngine::Math::Vector3 GetLightDirection() { return directionalLightData->direction; }
	float GetLightIntensity() { return directionalLightData->intensity; }
  int GetLightType() { return lightType_; }
	float GetLightShininess();
	// ポイントライト
	TuboEngine::Math::Vector3 GetPointLightPosition() { return pointLightData->position; }
	TuboEngine::Math::Vector4 GetPointLightColor() { return pointLightData->color; }
	float GetPointLightIntensity() { return pointLightData->intensity; }
	// スポットライト
	void GetSpotLightColor(TuboEngine::Math::Vector4& color) { color = spotLightData->color; }
	void GetSpotLightPosition(TuboEngine::Math::Vector3& position) { position = spotLightData->position; }
	void GetSpotLightDirection(TuboEngine::Math::Vector3& direction) { direction = spotLightData->direction; }
	float GetSpotLightIntensity() { return spotLightData->intensity; }
	float GetSpotLightDistance() { return spotLightData->distance; }
	float GetSpotLightDecay() { return spotLightData->decay; }
	float GetSpotLightCosAngle() { return spotLightData->cosAngle; }

	void SetCubeMapFilePath(const std::string& filePath) { cubeMapFilePath_ = filePath; }

private:
	//-------------------------------------------------------------------
	//		メンバ変数

	// モデル共通部分
	ModelCommon* modelCommon_ = nullptr;
	// モデルデータ
	TuboEngine::Model* model_ = nullptr;
	// カメラ
	TuboEngine::Camera* camera;

	// 座標のバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformMatrixResource;
	// 座標のバッファリソース内のデータを指すポインタ
	TransformationMatrix* transformMatrixData = nullptr;

	// 平行光源のバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
	// バッファリソース内のデータを指すポインタ
	DirectionalLight* directionalLightData = nullptr;

	// ポイントライトのバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
	// バッファリソース内のデータを指すポインタ
	PointLight* pointLightData = nullptr;

	// スポットライトのバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource;
	// バッファリソース内のデータを指すポインタ
	SpotLight* spotLightData = nullptr;

	// コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	// カメラ座標のバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraForGPUResource = nullptr;
	// カメラ座標のバッファリソース内のデータを指すポインタ
	CameraForGPU* cameraForGPUData = nullptr;

   // ライトの種類
	Microsoft::WRL::ComPtr<ID3D12Resource> lightTypeResource = nullptr;
	// ライトの種類のバッファリソース内のデータを指すポインタ
	LightType* lightTypeData = nullptr;
	int lightType_ = 1;

	// 3Dオブジェクトの座標
	Transform transform;
	// カメラ座標
	Transform cameraTransform;

	// キューブマップのSRVハンドル
	// デフォルト : ロストック・ラージ空港の4Kキューブマップ
	std::string cubeMapFilePath_ = "DDS/rostock_laage_airport_4k.dds";
};

} // namespace TuboEngine