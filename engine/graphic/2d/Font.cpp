#include "Font.h"
#include <iostream>
#include "SrvManager.h"

namespace TuboEngine {

bool Font::Initialize(const std::wstring& filePath, float fontSize) {
    fontSize_ = fontSize;

    // 1. DWriteCreateFactory で IDWriteFactory を作成
    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        &factory_
    );
    if (FAILED(hr)) {
        std::cerr << "Failed to create IDWriteFactory" << std::endl;
        return false;
    }

    // 2. CreateFontFileReference でフォントファイルを開く
    Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
    hr = factory_->CreateFontFileReference(filePath.c_str(), nullptr, &fontFile);
    if (FAILED(hr)) {
        std::cerr << "Failed to create font file reference: " << hr << std::endl;
        return false;
    }

    // 3. CreateFontFace で IDWriteFontFace を作成
    IDWriteFontFile* fontFiles[] = { fontFile.Get() };
    hr = factory_->CreateFontFace(
        DWRITE_FONT_FACE_TYPE_UNKNOWN,
        1,
        fontFiles,
        0,
        DWRITE_FONT_SIMULATIONS_NONE,
        &fontFace_
    );
    if (FAILED(hr)) {
        std::cerr << "Failed to create font face: " << hr << std::endl;
        return false;
    }

    // フォントメトリクスの取得
    DWRITE_FONT_METRICS fontMetrics;
    fontFace_->GetMetrics(&fontMetrics);
    
    // デザイン単位からピクセルへの変換スケール
    float scale = fontSize_ / fontMetrics.designUnitsPerEm;
    lineHeight_ = (fontMetrics.ascent + fontMetrics.descent + fontMetrics.lineGap) * scale;
    baseline_ = fontMetrics.ascent * scale;

    // アトラスの初期化
    atlasPixels_.resize(atlasWidth_ * atlasHeight_ * 4, 0); // RGBA
    if (!CreateAtlasResource()) {
        return false;
    }

    // 4. 必要なグリフ（ASCIIなど）のビットマップを生成し、アトラスにパッキング
    // 初期キャッシュとしてASCII文字を生成
    for (char32_t c = 0x20; c <= 0x7E; ++c) {
        AddGlyphToAtlas(c);
    }

    // 5. DirectXCommon を使ってアトラス用のテクスチャリソースを作成し、SRVを登録
    UpdateAtlasTexture();

    return true;
}

const Font::Glyph* Font::GetGlyph(char32_t codePoint) {
    auto it = glyphCache_.find(codePoint);
    if (it != glyphCache_.end()) {
        return &it->second;
    }

    // キャッシュにない場合は動的にグリフを生成してアトラスに追加する処理
    if (AddGlyphToAtlas(codePoint)) {
        UpdateAtlasTexture();
        return &glyphCache_[codePoint];
    }

    return nullptr;
}

bool Font::CreateAtlasResource() {
    auto dxCommon = DirectXCommon::GetInstance();
    auto device = dxCommon->GetDevice();

    // テクスチャリソースの作成
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = atlasWidth_;
    texDesc.Height = atlasHeight_;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&atlasResource_)
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to create atlas resource" << std::endl;
        return false;
    }

    // SRVの作成
    atlasSrvIndex_ = SrvManager::GetInstance()->Allocate();
    atlasSrvCPU_ = SrvManager::GetInstance()->GetCPUDescriptorHandle(atlasSrvIndex_);
    atlasSrvGPU_ = SrvManager::GetInstance()->GetGPUDescriptorHandle(atlasSrvIndex_);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(atlasResource_.Get(), &srvDesc, atlasSrvCPU_);

    return true;
}

bool Font::AddGlyphToAtlas(char32_t codePoint) {
    if (!fontFace_) return false;

    // 文字コードからグリフインデックスを取得
    uint32_t codePoint32 = static_cast<uint32_t>(codePoint);
    uint16_t glyphIndex = 0;
    HRESULT hr = fontFace_->GetGlyphIndices(&codePoint32, 1, &glyphIndex);
    if (FAILED(hr) || glyphIndex == 0) {
        return false; // グリフが存在しない
    }

    // グリフメトリクスの取得
    DWRITE_GLYPH_METRICS glyphMetrics;
    fontFace_->GetDesignGlyphMetrics(&glyphIndex, 1, &glyphMetrics);

    DWRITE_FONT_METRICS fontMetrics;
    fontFace_->GetMetrics(&fontMetrics);
    float scale = fontSize_ / fontMetrics.designUnitsPerEm;

    Glyph glyph;
    glyph.advanceX = glyphMetrics.advanceWidth * scale;
    glyph.offsetX = glyphMetrics.leftSideBearing * scale;
    // topSideBearing ではなく、ascent と topSideBearing を使ってベースラインからのオフセットを計算
    glyph.offsetY = (fontMetrics.ascent - glyphMetrics.topSideBearing) * scale;
    
    // グリフのビットマップを生成
    Microsoft::WRL::ComPtr<IDWriteGlyphRunAnalysis> glyphAnalysis;
    DWRITE_GLYPH_RUN glyphRun = {};
    glyphRun.fontFace = fontFace_.Get();
    glyphRun.fontEmSize = fontSize_;
    glyphRun.glyphCount = 1;
    glyphRun.glyphIndices = &glyphIndex;
    
    float advance = 0.0f;
    glyphRun.glyphAdvances = &advance;
    
    DWRITE_GLYPH_OFFSET offset = {0.0f, 0.0f};
    glyphRun.glyphOffsets = &offset;

    hr = factory_->CreateGlyphRunAnalysis(
        &glyphRun,
        1.0f, // pixelsPerDip
        nullptr, // transform
        DWRITE_RENDERING_MODE_ALIASED,
        DWRITE_MEASURING_MODE_NATURAL,
        0.0f, // baselineOriginX
        0.0f, // baselineOriginY
        &glyphAnalysis
    );

    if (FAILED(hr)) return false;

    RECT textureBounds;
    hr = glyphAnalysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &textureBounds);
    if (FAILED(hr)) return false;

    int width = textureBounds.right - textureBounds.left;
    int height = textureBounds.bottom - textureBounds.top;

    glyph.width = static_cast<float>(width);
    glyph.height = static_cast<float>(height);

    if (width > 0 && height > 0) {
        // アトラスのパッキング
        if (currentX_ + width > atlasWidth_) {
            currentX_ = 0;
            currentY_ += currentRowHeight_;
            currentRowHeight_ = 0;
        }

        if (currentY_ + height > atlasHeight_) {
            std::cerr << "Font atlas is full!" << std::endl;
            return false; // アトラスがいっぱい
        }

        // ビットマップの取得
        std::vector<uint8_t> alphaValues(width * height);
        hr = glyphAnalysis->CreateAlphaTexture(
            DWRITE_TEXTURE_ALIASED_1x1,
            &textureBounds,
            alphaValues.data(),
            static_cast<uint32_t>(alphaValues.size())
        );

        if (SUCCEEDED(hr)) {
            // アトラスにコピー (AのみからRGBAへ)
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    uint8_t alpha = alphaValues[y * width + x];
                    uint32_t atlasIdx = ((currentY_ + y) * atlasWidth_ + (currentX_ + x)) * 4;
                    atlasPixels_[atlasIdx + 0] = 255; // R
                    atlasPixels_[atlasIdx + 1] = 255; // G
                    atlasPixels_[atlasIdx + 2] = 255; // B
                    atlasPixels_[atlasIdx + 3] = alpha; // A
                }
            }

            // UV座標の計算
            glyph.uvLT = { static_cast<float>(currentX_) / atlasWidth_, static_cast<float>(currentY_) / atlasHeight_ };
            glyph.uvRB = { static_cast<float>(currentX_ + width) / atlasWidth_, static_cast<float>(currentY_ + height) / atlasHeight_ };

            currentX_ += width + 1; // 1ピクセルのパディング
            currentRowHeight_ = std::max(currentRowHeight_, static_cast<uint32_t>(height + 1));
        }
    } else {
        // 空白文字など
        glyph.uvLT = { 0.0f, 0.0f };
        glyph.uvRB = { 0.0f, 0.0f };
    }

    glyphCache_[codePoint] = glyph;
    return true;
}

void Font::UpdateAtlasTexture() {
    if (!atlasResource_) return;

    // テクスチャリソースにデータを書き込む
    HRESULT hr = atlasResource_->WriteToSubresource(
        0,
        nullptr,
        atlasPixels_.data(),
        atlasWidth_ * 4,
        atlasWidth_ * atlasHeight_ * 4
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to write to atlas texture" << std::endl;
    }
}

} // namespace TuboEngine
