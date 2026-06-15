#include "Sprite.h"
#include"SpriteCommon.h"
#include"Matrix.h"
#include"TextureManager.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"

void TuboEngine::Sprite::Initialize(std::string textureFilePath) {
	
	
	textureFilePath_ = textureFilePath;

	TextureManager::GetInstance()->LoadTexture(textureFilePath);

#pragma region SpriteResource

	vertexResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * 6);

	//頂点バッファビューを作成する

	//リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
	//1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);


	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	//三角形を2つ使って、四角形の作る
	/*---------------
		B-------D		1枚目 : ABCの三角形
		|		|		2枚目 : BDCの三角形
		|		|
		A-------C
	----------------*/
	//A
	vertexData[0].position = { 0.0f,360.0f,0.0f,1.0f };
	vertexData[0].texcoord = { 0.0f,1.0f };
	vertexData[0].normal = { 0.0f,0.0f,1.0f };

	//B

	vertexData[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexData[1].texcoord = { 0.0f,0.0f };
	vertexData[1].normal = { 0.0f,0.0f,1.0f };

	//C

	vertexData[2].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexData[2].texcoord = { 1.0f,1.0f };
	vertexData[2].normal = { 0.0f,0.0f,1.0f };


	//D
	vertexData[3].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexData[3].texcoord = { 1.0f,0.0f };
	vertexData[3].normal = { 0.0f,0.0f,1.0f };


	transformationMatrixResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
	//データを書き込む
	transformationMatrixData = nullptr;

	//書き込むためのアドレスを取得
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	//単位行列を書き込んでおく
	transformationMatrixData->WVP = TuboEngine::Math::MakeIdentity4x4();
	transformationMatrixData->World = TuboEngine::Math::MakeIdentity4x4();


#pragma endregion


#pragma region indexResourceSprite

	//WVP用のリソースを作る
	indexResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(uint32_t) * 6);
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();

	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;


	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	indexData[0] = 0;    indexData[1] = 1;   indexData[2] = 2;
	indexData[3] = 1;    indexData[4] = 3;   indexData[5] = 2;



#pragma endregion



#pragma region Material_Resource_Sprite
	//マテリアル用のリソースを作る。今回はColor1つ分のサイズを用意する
	materialResource = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(Material));
	//マテリアルにデータを書き込む
	//書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//今回は白を書き込んでみる
	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->enableLighting = false;
	materialData->uvTransform = TuboEngine::Math::MakeIdentity4x4();


#pragma endregion


	textureIndex = TextureManager::GetInstance()->GetSrvIndex(textureFilePath);

	AdjustTextureSize();
}

void TuboEngine::Sprite::Update() {

	//textureの位置
	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;


	// 左右反転
	if (isFlipX_) {
		left = 1.0f - anchorPoint.x;
		right = 0.0f - anchorPoint.x;
	}
	else {
		left = 0.0f - anchorPoint.x;
		right = 1.0f - anchorPoint.x;
	}
	//上下反転
	if (isFlipY_) {
		top = 1.0f - anchorPoint.x;
		bottom = 0.0f - anchorPoint.x;
	}
	else {
		top = 0.0f - anchorPoint.x;
		bottom = 1.0f - anchorPoint.x;
	}


	//テクスチャのメタデータを取得
	const DirectX::TexMetadata& metadata =
		TextureManager::GetInstance()->GetMetaData(textureFilePath_);

	//テクスチャの初期サイズ時の座標
	float tex_left = textureLeftTop_.x / metadata.width;
	float tex_right = (textureLeftTop_.x + textureSize_.x) / metadata.width;
	float tex_top = textureLeftTop_.y / metadata.height;
	float tex_bottom = (textureLeftTop_.y + textureSize_.y) / metadata.height;
	
	//テクスチャの初期サイズを呼び出す関数
	if (isAdjustTextureSize) {
		AdjustTextureSize();
	}

	/*---------------------------------------
	テクスチャの位置、画像位置, 法線ベクトル, 大きさ
	---------------------------------------*/

	transform.translate = { position.x,position.y,0.0f };
	transform.rotate = { 0.0f,0.0f,rotation };

	vertexData[0].position = { left,bottom,0.0f,1.0f };
	vertexData[0].texcoord = { tex_left,tex_bottom };
	vertexData[0].normal = { 0.0f,0.0f,-1.0f };

	vertexData[1].position = { left,top,0.0f,1.0f };
	vertexData[1].texcoord = { tex_left,tex_top };
	vertexData[1].normal = { 0.0f,0.0f,-1.0f };

	vertexData[2].position = { right,bottom,0.0f,1.0f };
	vertexData[2].texcoord = { tex_right,tex_bottom };
	vertexData[2].normal = { 0.0f,0.0f,-1.0f };

	vertexData[3].position = { right,top,0.0f,1.0f };
	vertexData[3].texcoord = { tex_right,tex_top };
	vertexData[3].normal = { 0.0f,0.0f,-1.0f };

	transform.scale = { size.x,size.y,1.0f };


	/*---------
	行列更新処理
	---------*/
	Matrix4x4 uvTransformMatrix = MakeAffineMatrix(uvTransFormMatrix.scale, uvTransFormMatrix.rotate, uvTransFormMatrix.translate);

	materialData->uvTransform = uvTransformMatrix;

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = TuboEngine::Math::MakeIdentity4x4();
	Matrix4x4 projectionMatrix =
	    TuboEngine::Math::MakeOrthographicMatrix(0.0f, 0.0f, float(TuboEngine::WinApp::GetInstance()->GetClientWidth()), float(TuboEngine::WinApp::GetInstance()->GetClientHeight()), 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
	transformationMatrixData->WVP = worldViewProjectionMatrix;
	transformationMatrixData->World = worldMatrix;



	commandList = TuboEngine::DirectXCommon::GetInstance()->GetCommandList();




}


void TuboEngine::Sprite::Draw() {

	

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);

	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

	//TransformationMatrixCBufferの設定
	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));
	//描画
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

}

void TuboEngine::Sprite::AdjustTextureSize() {

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath_);

	textureSize_.x = static_cast<float>(metadata.width);
	textureSize_.y = static_cast<float>(metadata.height);
	
	size = textureSize_;
}

// ImGuiでSpriteの全機能をまとめて操作・確認できる関数
void TuboEngine::Sprite::DrawImGui(const char* windowName) {
#ifdef USE_IMGUI

	const char* windowName_ = windowName;
	ImGui::Begin(windowName_);
    ImGui::Separator();
    ImGui::Text("Sprite ImGui コントロール"); // セクションタイトル
    // 位置
    TuboEngine::Math::Vector2 pos = GetPosition();
    if (ImGui::DragFloat2("Position", &pos.x, 1.0f)) {
        SetPosition(pos);
    }
    // 回転
    float rot = GetRotation();
    if (ImGui::DragFloat("Rotation", &rot, 0.01f)) {
        SetRotation(rot);
    }
    // サイズ
	TuboEngine::Math::Vector2 sz = GetSize();
    if (ImGui::DragFloat2("Size", &sz.x, 1.0f)) {
        SetSize(sz);
    }
    // アンカーポイント
	TuboEngine::Math::Vector2 anchor = GetAnchorPoint();
    if (ImGui::DragFloat2("AnchorPoint", &anchor.x, 0.01f, 0.0f, 1.0f)) {
        SetAnchorPoint(anchor);
    }
    // 左右フリップ
    bool flipX = GetFlipX();
    if (ImGui::Checkbox("FlipX", &flipX)) {
        SetFlipX(flipX);
    }
    // 上下フリップ
    bool flipY = GetFlipY();
    if (ImGui::Checkbox("FlipY", &flipY)) {
        SetFlipY(flipY);
    }
    // テクスチャ左上座標
	TuboEngine::Math::Vector2 texLT = GetTextureLeftTop();
    if (ImGui::DragFloat2("TextureLeftTop", &texLT.x, 1.0f)) {
        SetTextureLeftTop(texLT);
    }
    // テクスチャ切り出しサイズ
	TuboEngine::Math::Vector2 texSz = GetTextureSize();
    if (ImGui::DragFloat2("TextureSize", &texSz.x, 1.0f)) {
        SetTextureSize(texSz);
    }
    // テクスチャ初期サイズ調整フラグ
    bool adjust = GetIsAdjustTextureSize();
    if (ImGui::Checkbox("AdjustTextureSize", &adjust)) {
        SetGetIsAdjustTextureSize(adjust);
    }
    // 色
    Vector4 color = GetColor();
    if (ImGui::ColorEdit4("Color", &color.x)) {
        SetColor(color);
    }
    // コメント: ここでSpriteの全てのプロパティをImGuiで操作できます
	ImGui::End(); // ウィンドウ終了

	#endif // USE_IMGUI
}
