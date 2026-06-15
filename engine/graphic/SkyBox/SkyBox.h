#pragma once
#include "SkyBoxCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include <vector>
#include <wrl.h>
#include <string>
#include"VertexData.h"
#include"TransformationMatrix.h"
#include"Material.h"

namespace TuboEngine {
class SkyBox {
public:
	void Initialize(const std::string& textureFilePath);
	void Update();
	void Draw();

public:
	void SetCamera(TuboEngine::Camera* camera) { camera_ = camera; }

	void SetTransform(const Transform& transform) { this->transform = transform; }
	const Transform& GetTransform() const { return transform; }

	void SetPosition(const TuboEngine::Math::Vector3& position) { transform.translate = position; }
	void SetRotation(const TuboEngine::Math::Vector3& rotation) { transform.rotate = rotation; }
	void SetScale(const TuboEngine::Math::Vector3& scale) { transform.scale = scale; }

	void SetTextureFilePath(const std::string& textureFilePath) { textureFilePath_ = textureFilePath; }
	const std::string& GetTextureFilePath() const { return textureFilePath_; }

private:
	TuboEngine::Camera* camera_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;
	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// バッファリソース内のデータを指すポインタ
	TransformationMatrix* transformationMatrixData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
	std::string textureFilePath_;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	// バッファリソース内のデータを指すポインタ
	Material* materialData = nullptr;

	// 大きさ、回転、位置
	Transform transform{
	    {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
};
} // namespace TuboEngine