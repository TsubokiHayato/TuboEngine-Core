#include "Object3d.h"
#include "Camera.h"
#include "Matrix.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "numbers"
#include <array>

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

namespace {
struct SharedLightResources {
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
	DirectionalLight* directionalLightData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
	PointLight* pointLightData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource;
	SpotLight* spotLightData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraForGPUResource;
   TuboEngine::CameraForGPU* cameraForGPUData = nullptr;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 6> lightTypeResources{};
	std::array<LightType*, 6> lightTypeData{};
	bool initialized = false;

	void Initialize() {
		if (initialized) {
			return;
		}

		directionalLightResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(DirectionalLight));
		directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
		directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
		directionalLightData->direction = {0.0f, -1.0f, 0.0f};
		directionalLightData->intensity = 1.0f;

		pointLightResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(PointLight));
		pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
		pointLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
		pointLightData->position = {0.0f, 1.0f, 0.0f};
		pointLightData->intensity = 1.0f;

		spotLightResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(SpotLight));
		spotLightResource->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData));
		spotLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
		spotLightData->position = {2.0f, 1.25f, 0.0f};
		spotLightData->direction = TuboEngine::Math::Vector3::Normalize({-1.0f, -1.0f, 0.0f});
		spotLightData->intensity = 4.0f;
		spotLightData->distance = 7.0f;
		spotLightData->decay = 2.0f;
		spotLightData->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);

        cameraForGPUResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TuboEngine::CameraForGPU));
		cameraForGPUResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraForGPUData));
		cameraForGPUData->worldPosition = {};

		for (int i = 0; i < static_cast<int>(lightTypeResources.size()); ++i) {
			lightTypeResources[i] = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(LightType));
			lightTypeResources[i]->Map(0, nullptr, reinterpret_cast<void**>(&lightTypeData[i]));
			lightTypeData[i]->type = i;
		}

		initialized = true;
	}

	ID3D12Resource* GetLightTypeResource(int type) {
		if (type < 0 || type >= static_cast<int>(lightTypeResources.size())) {
			type = 0;
		}
		return lightTypeResources[type].Get();
	}

	void Release() {
		if (!initialized) {
			return;
		}

		if (directionalLightResource) {
			directionalLightResource->Unmap(0, nullptr);
		}
		directionalLightData = nullptr;
		directionalLightResource.Reset();

		if (pointLightResource) {
			pointLightResource->Unmap(0, nullptr);
		}
		pointLightData = nullptr;
		pointLightResource.Reset();

		if (spotLightResource) {
			spotLightResource->Unmap(0, nullptr);
		}
		spotLightData = nullptr;
		spotLightResource.Reset();

		if (cameraForGPUResource) {
			cameraForGPUResource->Unmap(0, nullptr);
		}
		cameraForGPUData = nullptr;
		cameraForGPUResource.Reset();

		for (size_t i = 0; i < lightTypeResources.size(); ++i) {
			if (lightTypeResources[i]) {
				lightTypeResources[i]->Unmap(0, nullptr);
			}
			lightTypeData[i] = nullptr;
			lightTypeResources[i].Reset();
		}

		initialized = false;
	}
};

SharedLightResources& GetSharedLightResources() {
	static SharedLightResources resources;
	resources.Initialize();
	return resources;
}
} // namespace

namespace TuboEngine {
void SharedLightResourcesRelease() {
	GetSharedLightResources().Release();
}
} // namespace TuboEngine

void TuboEngine::Object3d::Initialize(std::string modelFileNamePath) {
	
	this->camera = Object3dCommon::GetInstance()->GetDefaultCamera();

	ModelManager::GetInstance()->LoadModel(modelFileNamePath);
	SetModel(modelFileNamePath);

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

	auto& shared = GetSharedLightResources();
	directionalLightResource = shared.directionalLightResource;
	directionalLightData = shared.directionalLightData;
	pointLightResource = shared.pointLightResource;
	pointLightData = shared.pointLightData;
	spotLightResource = shared.spotLightResource;
	spotLightData = shared.spotLightData;
	cameraForGPUResource = shared.cameraForGPUResource;
	cameraForGPUData = shared.cameraForGPUData;
   lightTypeResource = shared.GetLightTypeResource(lightType_);



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
}

void TuboEngine::Object3d::Update() {

	cameraForGPUData->worldPosition = camera->GetTranslate();

	// 行列を更新する
	TuboEngine::Math::Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	TuboEngine::Math::Matrix4x4 cameraMatrix = MakeAffineMatrix(camera->GetScale(), camera->GetRotation(), camera->GetTranslate());
	TuboEngine::Math::Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	TuboEngine::Math::Matrix4x4 projectionMatrix =
	    TuboEngine::Math::MakePerspectiveMatrix(0.45f, float(TuboEngine::WinApp::GetInstance()->GetClientWidth()) / float(TuboEngine::WinApp::GetInstance()->GetClientHeight()), 0.1f, 100.0f);
	TuboEngine::Math::Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

	if (camera) {
		const Matrix4x4& viewProjectionMatrix = camera->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}
	// 行列を更新する
	TuboEngine::Math::Matrix4x4 localMatrix = model_->GetRootNodeLocalMatrix();
	if (camera) {
		transformMatrixData->WVP = model_->GetRootNodeLocalMatrix() * worldMatrix * camera->GetViewProjectionMatrix();
	} else {
		transformMatrixData->WVP = model_->GetRootNodeLocalMatrix() * worldMatrix * Multiply(viewMatrix, projectionMatrix);
	}
	transformMatrixData->World = model_->GetRootNodeLocalMatrix() * worldMatrix;

	commandList = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();
}

void TuboEngine::Object3d::Draw() {

	// TransformMatrix (b0, VertexShader)
	commandList->SetGraphicsRootConstantBufferView(1, transformMatrixResource->GetGPUVirtualAddress());

	// gTexture (t0, PixelShader)はModelクラスにある。
	
	// 例: キューブマップSRVをt1（gCubeTexture）にバインド
	// gCubeTexture (t1, PixelShader)
	commandList->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance()->GetSrvHandleGPU(cubeMapFilePath_));
	// DirectionalLight (b1, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(4, directionalLightResource->GetGPUVirtualAddress());
	// Camera (b2, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(5, cameraForGPUResource->GetGPUVirtualAddress());
	// LightType (b3, PixelShader)
   auto& shared = GetSharedLightResources();
	lightTypeResource = shared.GetLightTypeResource(lightType_);
	commandList->SetGraphicsRootConstantBufferView(6, lightTypeResource->GetGPUVirtualAddress());
	// PointLight (b4, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(7, pointLightResource->GetGPUVirtualAddress());
	// SpotLight (b5, PixelShader)
	commandList->SetGraphicsRootConstantBufferView(8, spotLightResource->GetGPUVirtualAddress());

	


	// 3Dモデルが割り当てられていれば描画
	if (model_) {
		model_->Draw();
	} else {
		// 3Dモデルが割り当てられていない場合はブレイクさせる
		assert(0);
	}
}

void TuboEngine::Object3d::DrawImGui(const char* windowName) {
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
    static const char* lightTypeItems[] = {
        "Directional", "Phong", "Blinn-Phong", "Point", "Spot", "EnvironmentMap"
    };
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

void TuboEngine::Object3d::SetLightShininess(float shininess) { model_->SetModelShininess(shininess); }

void TuboEngine::Object3d::SetModel(const std::string& filePath) { model_ = ModelManager::GetInstance()->FindModel(filePath); }

void TuboEngine::Object3d::SetModelColor(const Vector4& color) { model_->SetModelColor(color); }

Vector4 TuboEngine::Object3d::GetModelColor() { return model_->GetModelColor(); }

float TuboEngine::Object3d::GetLightShininess() { return model_->GetModelShininess(); }
