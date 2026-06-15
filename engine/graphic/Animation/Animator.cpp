#include "Animator.h"
#include "ImGuiManager.h"
#include "Matrix.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "numbers"
#include <cassert>
#include <cmath>

// 初期化
void TuboEngine::Animator::Initialize(std::string modelFileNamePath) {

	this->camera = TuboEngine::Object3dCommon::GetInstance()->GetDefaultCamera();

	ModelManager::GetInstance()->LoadModel(modelFileNamePath);
	SetModel(modelFileNamePath);

	animation_ = LoadAnimation("Resources/Models", modelFileNamePath);
	animationNodeName_ = modelFileNamePath; 
	animationTime_ = 0.0f;
	animationLoop_ = true;

#pragma region TransformMatrixResourced

	// WVP用のリソースを作る
	transformMatrixResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
	// データを書き込む
	transformMatrixData = nullptr;
	// 書き込むためのアドレスを取得
	transformMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformMatrixData));
	// 単位行列を書き込んでいく
	transformMatrixData->WVP = TuboEngine::Math::MakeIdentity4x4();
	transformMatrixData->World = TuboEngine::Math::MakeIdentity4x4();

#pragma endregion TransformMatrixResource

#pragma region DirectionalLightData
	// 平行光源用用のリソースを作る。今回はColor1つ分のサイズを用意する
	directionalLightResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(DirectionalLight));
	// 平行光源用にデータを書き込む
	directionalLightData = nullptr;
	// 書き込むためのアドレスを取得
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	// デフォルト値
	directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionalLightData->direction = {0.0f, -1.0f, 0.0f};
	directionalLightData->intensity = 1.0f;

#pragma endregion

#pragma region PointLight

	// ポイントライト用用のリソースを作る。今回はColor1つ分のサイズを用意する
	pointLightResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(PointLight));
	// 平行光源用にデータを書き込む
	pointLightData = nullptr;
	// 書き込むためのアドレスを取得
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
	// デフォルト値
	pointLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	pointLightData->position = {0.0f, 1.0f, 0.0f};
	pointLightData->intensity = 1.0f;

#pragma endregion

#pragma region SpotLight

	// スポットライト用用のリソースを作る。今回はColor1つ分のサイズを用意する
	spotLightResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(SpotLight));
	// 平行光源用にデータを書き込む
	spotLightData = nullptr;
	// 書き込むためのアドレスを取得
	spotLightResource->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData));
	// デフォルト値
	spotLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	spotLightData->position = {2.0f, 1.25f, 0.0f};
	spotLightData->direction = TuboEngine::Math::Vector3::Normalize({-1.0f, -1.0f, 0.0f});
	spotLightData->intensity = 4.0f;
	spotLightData->distance = 7.0f;
	spotLightData->decay = 2.0f;
	spotLightData->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);

#pragma endregion

#pragma region cameraWorldPos
	// 平行光源用用のリソースを作る。今回はColor1つ分のサイズを用意する
	cameraForGPUResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(CameraForGPU));
	// 平行光源用にデータを書き込む
	cameraForGPUData = nullptr;
	// 書き込むためのアドレスを取得
	cameraForGPUResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraForGPUData));

	cameraForGPUData->worldPosition = {};
#pragma endregion

#pragma region LightType
	// ライトの種類
	lightTypeResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(LightType));
	// 平行光源用にデータを書き込む
	lightTypeData = nullptr;
	// 書き込むためのアドレスを取得
	lightTypeResource->Map(0, nullptr, reinterpret_cast<void**>(&lightTypeData));

	// デフォルト値
	lightTypeData->type = 0;

#pragma endregion

	// transform変数を作る
	transform = {
	    {1.0f, 1.0f, 1.0f},
	    {0.0f, 0.0f, 0.0f},
	    {0.0f, 0.0f, 0.0f},
	};
	cameraTransform = {
	    {1.0f, 1.0f, 1.0f  },
	    {0.3f, 0.0f, 0.0f  },
	    {0.0f, 4.0f, -10.0f},
	};

	// アニメーションノード名一覧を取得
	std::vector<std::string> nodeNames = GetAnimationNodeNames("Resources/Models", modelFileNamePath);
	// 例: 最初のノード名を使う
	if (!nodeNames.empty()) {
	    animationNodeName_ = nodeNames[0];
	} else {
	    animationNodeName_ = ""; // アニメーションが無い場合
	}
}
void TuboEngine::Animator::Update() {
    animationTime_ += 1.0f / 60.0f; // 60FPS
    animationTime_ = std::fmod(animationTime_, animation_.duration);

    NodeAnimation& rootNodeAnimation = animation_.nodeAnimations[animationNodeName_];
	TuboEngine::Math::Vector3 translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime_);
	TuboEngine::Math::Quaternion rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime_);
	TuboEngine::Math::Vector3 scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime_);

    // アニメーション値をTransformに反映
    transform.translate = translate;
    transform.rotate = rotate.ToEuler();
    transform.scale = scale;

    cameraForGPUData->worldPosition = camera->GetTranslate();

    // 行列を更新する
	TuboEngine::Math::Matrix4x4 worldMatrix = TuboEngine::Math::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	TuboEngine::Math::Matrix4x4 cameraMatrix = TuboEngine::Math::MakeAffineMatrix(camera->GetScale(), camera->GetRotation(), camera->GetTranslate());
	TuboEngine::Math::Matrix4x4 viewMatrix = TuboEngine::Math::Inverse(cameraMatrix);
	TuboEngine::Math::Matrix4x4 projectionMatrix =
	    TuboEngine::Math::MakePerspectiveMatrix(0.45f,
	        float(TuboEngine::WinApp::GetInstance()->GetClientWidth()) /
	        float(TuboEngine::WinApp::GetInstance()->GetClientHeight()),
	        0.1f, 100.0f);
	TuboEngine::Math::Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

    if (camera) {
        const TuboEngine::Math::Matrix4x4& viewProjectionMatrix = camera->GetViewProjectionMatrix();
        worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
    } else {
        worldViewProjectionMatrix = worldMatrix;
    }

    transformMatrixData->WVP = model_->GetRootNodeLocalMatrix() * worldMatrix * Multiply(viewMatrix, projectionMatrix);
    transformMatrixData->World = model_->GetRootNodeLocalMatrix() * worldMatrix;

    commandList = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();
}

// モデルの設定
void TuboEngine::Animator::SetModel(TuboEngine::Model* model) {
	assert(model);
	this->model_ = model;
}

void TuboEngine::Animator::SetModel(const std::string& filePath) {
	this->model_ = ModelManager::GetInstance()->FindModel(filePath);
}

// 描画処理
void TuboEngine::Animator::Draw() {
	// TransformMatrix (b0, VertexShader)
	commandList->SetGraphicsRootConstantBufferView(1, transformMatrixResource->GetGPUVirtualAddress());

	// gTexture (t0, PixelShader)はModelクラスにある。

	// 例: キューブマップSRVをt1（gCubeTexture）にバインド
	// gCubeTexture (t1, PixelShader)
	commandList->SetGraphicsRootDescriptorTable(3, TuboEngine::TextureManager::GetInstance()->GetSrvHandleGPU(cubeMapFilePath_));
	// DirectionalLight (b1, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(4, directionalLightResource->GetGPUVirtualAddress());
	// Camera (b2, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(5, cameraForGPUResource->GetGPUVirtualAddress());
	// LightType (b3, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(6, lightTypeResource->GetGPUVirtualAddress());
	// PointLight (b4, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(7, pointLightResource->GetGPUVirtualAddress());
	// SpotLight (b5, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(8, spotLightResource->GetGPUVirtualAddress());

	if (model_) {
		model_->Draw();
	}
}

// ImGui描画
void TuboEngine::Animator::DrawImGui(const char* windowName) {

#ifdef USE_IMGUI
	ImGui::Begin(windowName);

	ImGui::Separator();
	ImGui::Text("Object3d ImGui コントロール");
	// 位置
	TuboEngine::Math::Vector3 pos = GetPosition();
	if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
		SetPosition(pos);
	}
	// 回転
	TuboEngine::Math::Vector3 rot = GetRotation();
	if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) {
		SetRotation(rot);
	}
	// スケール
	TuboEngine::Math::Vector3 scale = GetScale();
	if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) {
		SetScale(scale);
	}
	// モデル色
	TuboEngine::Math::Vector4 color = GetModelColor();
	if (ImGui::ColorEdit4("Color", &color.x)) {
		SetModelColor(color);
	}

	// ライトタイプ選択
	static const char* lightTypeItems[] = {"Directional", "Phong", "Blinn-Phong", "Point", "Spot", "EnvironmentMap"};
	int lightType = GetLightType();
	if (ImGui::Combo("LightType", &lightType, lightTypeItems, IM_ARRAYSIZE(lightTypeItems))) {
		SetLightType(lightType);
	}

	// 必要なライトだけImGui表示
	if (lightType == 0) { // Directional
		ImGui::Separator();
		ImGui::Text("Directional Light");
		ImGui::ColorEdit4("LightColor", &directionalLightData->color.x);
		ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.1f);
		ImGui::DragFloat("LightIntensity", &directionalLightData->intensity, 0.1f);
		float shininess = model_->GetModelShininess();
		if (ImGui::DragFloat("Shininess", &shininess)) {
			model_->SetModelShininess(shininess);
		}
	} else if (lightType == 1) { // Phong
		ImGui::Separator();
		ImGui::Text("Phong Reflection");
		ImGui::ColorEdit4("LightColor", &directionalLightData->color.x);
		ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.1f);
		ImGui::DragFloat("LightIntensity", &directionalLightData->intensity, 0.1f);
		float shininess = model_->GetModelShininess();
		if (ImGui::DragFloat("Shininess", &shininess)) {
			model_->SetModelShininess(shininess);
		}
	} else if (lightType == 2) { // Blinn-Phong
		ImGui::Separator();
		ImGui::Text("Blinn-Phong Reflection");
		ImGui::ColorEdit4("LightColor", &directionalLightData->color.x);
		ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.1f);
		ImGui::DragFloat("LightIntensity", &directionalLightData->intensity, 0.1f);
		float shininess = model_->GetModelShininess();
		if (ImGui::DragFloat("Shininess", &shininess)) {
			model_->SetModelShininess(shininess);
		}
	} else if (lightType == 3) { // Point
		ImGui::Separator();
		ImGui::Text("Point Light");
		ImGui::ColorEdit4("PointLightColor", &pointLightData->color.x);
		ImGui::DragFloat3("PointLightPosition", &pointLightData->position.x, 0.1f);
		ImGui::DragFloat("PointLightIntensity", &pointLightData->intensity, 0.1f);
	} else if (lightType == 4) { // Spot
		ImGui::Separator();
		ImGui::Text("Spot Light");
		ImGui::ColorEdit4("SpotLightColor", &spotLightData->color.x);
		ImGui::DragFloat3("SpotLightPosition", &spotLightData->position.x, 0.1f);
		ImGui::DragFloat3("SpotLightDirection", &spotLightData->direction.x, 0.1f);
		ImGui::DragFloat("SpotLightIntensity", &spotLightData->intensity, 0.1f);
		ImGui::DragFloat("SpotLightDistance", &spotLightData->distance, 0.1f);
		ImGui::DragFloat("SpotLightDecay", &spotLightData->decay, 0.1f);
		ImGui::DragFloat("SpotLightCosAngle", &spotLightData->cosAngle, 0.1f);
	} else if (lightType == 5) { // EnvironmentMap
		ImGui::Separator();
		ImGui::Text("Environment Mapping");
		ImGui::ColorEdit4("LightColor", &directionalLightData->color.x);
		ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.1f);
		ImGui::DragFloat("LightIntensity", &directionalLightData->intensity, 0.1f);
		float shininess = model_->GetModelShininess();
		if (ImGui::DragFloat("Shininess", &shininess)) {
			model_->SetModelShininess(shininess);
		}
		float envCoef = model_->GetModelEnvironmentCoefficient();
		if (ImGui::SliderFloat("Environment Coefficient", &envCoef, 0.0f, 1.0f)) {
			model_->SetModelEnvironmentCoefficient(envCoef);
		}
	}
	ImGui::End();
#endif // USE_IMGUI
}

// モデルの色設定
void TuboEngine::Animator::SetModelColor(const TuboEngine::Math::Vector4& color) {
	if (model_) {
		model_->SetModelColor(color);
	}
}

// モデルの色取得
TuboEngine::Math::Vector4 TuboEngine::Animator::GetModelColor() {
	if (model_) {
		return model_->GetModelColor();
	}
	return TuboEngine::Math::Vector4{};
}

//Vector3
TuboEngine::Math::Vector3 TuboEngine::Animator::CalculateValue(const std::vector<KeyFrame<TuboEngine::Math::Vector3>>& keyframes, float time) {
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}
	for (size_t i = 0; i < keyframes.size() - 1; ++i) {
		size_t next = i + 1;
		if (time < keyframes[next].time) {
			float t = (time - keyframes[i].time) / (keyframes[next].time - keyframes[i].time);
			return TuboEngine::Math::Vector3::Lerp(keyframes[i].value, keyframes[next].value, t);
		}
	}
	return keyframes.back().value;
}
// Quaternion
TuboEngine::Math::Quaternion TuboEngine::Animator::CalculateValue(const std::vector<KeyFrame<TuboEngine::Math::Quaternion>>& keyframes, float time) {
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}
	for (size_t i = 0; i < keyframes.size() - 1; ++i) {
		size_t next = i + 1;
		if (time < keyframes[next].time) {
			float t = (time - keyframes[i].time) / (keyframes[next].time - keyframes[i].time);
			// QuaternionのSlerp（球面線形補間）
			return TuboEngine::Math::Quaternion::Slerp(keyframes[i].value, keyframes[next].value, t);
		}
	}
	return keyframes.back().value;
}