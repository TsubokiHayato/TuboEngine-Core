#pragma once
#include <string>
#include <memory>
#include <wrl.h>
#include "DirectXCommon.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Font.h"
#include "VertexData.h"
#include "Material.h"
#include "TransformationMatrix.h"
#include "Transform.h"

namespace TuboEngine {

class TextObject {
public:
    TextObject() = default;
    ~TextObject() = default;

    void Initialize();

    void SetText(const std::string& text);  // UTF-8 -> UTF-32 変換
    void SetPosition(const Math::Vector2& pos);
    void SetColor(const Math::Vector4& color);
    void SetScale(float scale);
    void SetFont(Font* font);

    // 揃え設定（0: Left/Top, 1: Center/Middle, 2: Right/Bottom）
    void SetHorizontalAlign(int align) { horizontalAlign_ = align; dirty_ = true; }
    void SetVerticalAlign(int align) { verticalAlign_ = align; dirty_ = true; }

    // Getter
    const Math::Vector2& GetPosition() const { return position_; }
    const Math::Vector4& GetColor() const { return color_; }
    float GetScale() const { return scale_; }
    Font* GetFont() const { return font_; }

    void Update();  // 文字列やパラメータ変更時に頂点を更新
    void Draw();    // Sprite と同じパイプラインで描画

private:
    std::u32string text_;
    Math::Vector2 position_{0.0f, 0.0f};
    Math::Vector4 color_{1.0f, 1.0f, 1.0f, 1.0f};
    float scale_ = 1.0f;
    Font* font_ = nullptr;

    // 揃えモード
    int horizontalAlign_ = 0; // 0: Left, 1: Center, 2: Right
    int verticalAlign_   = 0; // 0: Top, 1: Middle, 2: Bottom

    // 頂点バッファ / インデックスバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    VertexData* vertexData_ = nullptr;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW ibView_{};
    uint32_t* indexData_ = nullptr;

    // Transform / Material 相当の CBuffer
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    Transform transform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
    Transform uvTransform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

    uint32_t maxCharacters_ = 256; // 最大文字数
    uint32_t currentCharacterCount_ = 0;

    bool dirty_ = true;  // 文字列変更時に true → Update() で再生成
};

} // namespace TuboEngine
