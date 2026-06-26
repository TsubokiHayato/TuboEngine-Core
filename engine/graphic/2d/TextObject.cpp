#include "TextObject.h"
#include <iostream>
#include <Windows.h>
#include "DirectXCommon.h"
#include "Matrix.h"
#include "WinApp.h"

namespace TuboEngine {

void TextObject::Initialize() {
    // 頂点バッファの作成 (最大文字数 * 4頂点)
    vertexBuffer_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * 4 * maxCharacters_);
    vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * 4 * maxCharacters_;
    vbView_.StrideInBytes = sizeof(VertexData);
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    // インデックスバッファの作成 (最大文字数 * 6インデックス)
    indexBuffer_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(uint32_t) * 6 * maxCharacters_);
    ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(uint32_t) * 6 * maxCharacters_;
    ibView_.Format = DXGI_FORMAT_R32_UINT;
    indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

    // インデックスデータの初期化 (固定パターン)
    for (uint32_t i = 0; i < maxCharacters_; ++i) {
        indexData_[i * 6 + 0] = i * 4 + 0;
        indexData_[i * 6 + 1] = i * 4 + 1;
        indexData_[i * 6 + 2] = i * 4 + 2;
        indexData_[i * 6 + 3] = i * 4 + 1;
        indexData_[i * 6 + 4] = i * 4 + 3;
        indexData_[i * 6 + 5] = i * 4 + 2;
    }

    // マテリアルバッファの作成
    materialResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = color_;
    materialData_->enableLighting = false;
	materialData_->uvTransform = TuboEngine::Math::MakeIdentity4x4();

    // 変換行列バッファの作成
    transformationMatrixResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
	transformationMatrixData_->WVP = TuboEngine::Math::MakeIdentity4x4();
	transformationMatrixData_->World = TuboEngine::Math::MakeIdentity4x4();
}

void TextObject::SetText(const std::string& text) {
    if (text.empty()) {
        if (!text_.empty()) {
            text_.clear();
            dirty_ = true;
        }
        return;
    }

    // UTF-8 から UTF-16 への変換
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), &wstrTo[0], size_needed);

    // UTF-16 から UTF-32 への変換 (簡易的なサロゲートペア対応)
    std::u32string newText;
    for (size_t i = 0; i < wstrTo.length(); ++i) {
        wchar_t c = wstrTo[i];
        if (c >= 0xD800 && c <= 0xDBFF) {
            if (i + 1 < wstrTo.length()) {
                wchar_t c2 = wstrTo[i + 1];
                if (c2 >= 0xDC00 && c2 <= 0xDFFF) {
                    char32_t cp = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
                    newText.push_back(cp);
                    ++i;
                }
            }
        } else {
            newText.push_back(c);
        }
    }

    if (text_ != newText) {
        text_ = newText;
        dirty_ = true;
    }
}

void TextObject::SetPosition(const Math::Vector2& pos) {
    if (position_.x != pos.x || position_.y != pos.y) {
        position_ = pos;
        dirty_ = true;
    }
}

void TextObject::SetColor(const Math::Vector4& color) {
    if (color_.x != color.x || color_.y != color.y || color_.z != color.z || color_.w != color.w) {
        color_ = color;
        if (materialData_) {
            materialData_->color = color_;
        }
    }
}

void TextObject::SetScale(float scale) {
    if (scale_ != scale) {
        scale_ = scale;
        dirty_ = true;
    }
}

void TextObject::SetFont(Font* font) {
    if (font_ != font) {
        font_ = font;
        dirty_ = true;
    }
}

void TextObject::Update() {
    if (!dirty_ || !font_) return;

    currentCharacterCount_ = 0;
    float cursorX = 0.0f;
    float cursorY = 0.0f;

    // まずはテキスト全体のサイズを計算
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    {
        float lineWidth = 0.0f;
        float maxLineWidth = 0.0f;
        float height = font_->GetLineHeight();
        for (char32_t c : text_) {
            if (c == U'\n') {
                if (lineWidth > maxLineWidth) { maxLineWidth = lineWidth; }
                lineWidth = 0.0f;
                height += font_->GetLineHeight();
                continue;
            }
            const Font::Glyph* glyph = font_->GetGlyph(c);
            if (!glyph) { continue; }
            lineWidth += glyph->advanceX;
        }
        if (lineWidth > maxLineWidth) { maxLineWidth = lineWidth; }
        totalWidth = maxLineWidth * scale_;
        totalHeight = height * scale_;
    }
    totalWidth_ = totalWidth;
    totalHeight_ = totalHeight;

    // 揃えに応じた原点オフセット
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    // 横方向
    switch (horizontalAlign_) {
    case 1: // Center
        offsetX = -totalWidth * 0.5f;
        break;
    case 2: // Right
        offsetX = -totalWidth;
        break;
    case 0: // Left
    default:
        offsetX = 0.0f;
        break;
    }

    // 縦方向
    switch (verticalAlign_) {
    case 1: // Middle
        offsetY = -totalHeight * 0.5f;
        break;
    case 2: // Bottom
        offsetY = -totalHeight;
        break;
    case 0: // Top
    default:
        offsetY = 0.0f;
        break;
    }

    cursorX = 0.0f;
    cursorY = 0.0f;

    for (char32_t c : text_) {
        if (currentCharacterCount_ >= maxCharacters_) break;

        if (c == U'\n') {
            cursorX = 0.0f;
            cursorY += font_->GetLineHeight();
            continue;
        }

        const Font::Glyph* glyph = font_->GetGlyph(c);
        if (!glyph) continue;

        // 頂点データの生成
        uint32_t vIndex = currentCharacterCount_ * 4;
        
        float left = (cursorX + glyph->offsetX) * scale_ + offsetX;
        float right = left + glyph->width * scale_;
        float top = (cursorY + font_->GetBaseline() - glyph->offsetY) * scale_ + offsetY;
        float bottom = top + glyph->height * scale_;

        // A (左下)
        vertexData_[vIndex + 0].position = { left, bottom, 0.0f, 1.0f };
        vertexData_[vIndex + 0].texcoord = { glyph->uvLT.x, glyph->uvRB.y };
        vertexData_[vIndex + 0].normal = { 0.0f, 0.0f, -1.0f };

        // B (左上)
        vertexData_[vIndex + 1].position = { left, top, 0.0f, 1.0f };
        vertexData_[vIndex + 1].texcoord = { glyph->uvLT.x, glyph->uvLT.y };
        vertexData_[vIndex + 1].normal = { 0.0f, 0.0f, -1.0f };

        // C (右下)
        vertexData_[vIndex + 2].position = { right, bottom, 0.0f, 1.0f };
        vertexData_[vIndex + 2].texcoord = { glyph->uvRB.x, glyph->uvRB.y };
        vertexData_[vIndex + 2].normal = { 0.0f, 0.0f, -1.0f };

        // D (右上)
        vertexData_[vIndex + 3].position = { right, top, 0.0f, 1.0f };
        vertexData_[vIndex + 3].texcoord = { glyph->uvRB.x, glyph->uvLT.y };
        vertexData_[vIndex + 3].normal = { 0.0f, 0.0f, -1.0f };

        cursorX += glyph->advanceX;
        currentCharacterCount_++;
    }

    // 行列の更新
    transform_.translate = { position_.x, position_.y, 0.0f };
    transform_.scale = { 1.0f, 1.0f, 1.0f }; // スケールは頂点に反映済み

    Matrix4x4 uvTransformMatrix = MakeAffineMatrix(uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
    materialData_->uvTransform = uvTransformMatrix;
    materialData_->color = color_;

    Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 viewMatrix = TuboEngine::Math::MakeIdentity4x4();
	Matrix4x4 projectionMatrix = TuboEngine::Math::MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::GetInstance()->GetClientWidth()), float(WinApp::GetInstance()->GetClientHeight()), 0.0f, 100.0f);
    Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
    
    transformationMatrixData_->WVP = worldViewProjectionMatrix;
    transformationMatrixData_->World = worldMatrix;

    dirty_ = false;
}

void TextObject::Draw() {
    if (!font_ || currentCharacterCount_ == 0) return;

    auto commandList = DirectXCommon::GetInstance()->GetCommandList();

    commandList->IASetVertexBuffers(0, 1, &vbView_);
    commandList->IASetIndexBuffer(&ibView_);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, font_->GetAtlasSrv());

    commandList->DrawIndexedInstanced(currentCharacterCount_ * 6, 1, 0, 0, 0);
}

} // namespace TuboEngine
