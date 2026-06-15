#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <dwrite.h>
#include <wrl.h>
#include <vector>
#include "DirectXCommon.h"
#include "Vector2.h"

#pragma comment(lib, "dwrite.lib")

namespace TuboEngine {

class Font {
public:
    struct Glyph {
        float advanceX;
        float offsetX, offsetY;    // bearing
        float width, height;
        Math::Vector2 uvLT, uvRB;  // フォントアトラス上のUV
    };

    Font() = default;
    ~Font() = default;

    bool Initialize(const std::wstring& filePath, float fontSize);
    const Glyph* GetGlyph(char32_t codePoint);

    D3D12_GPU_DESCRIPTOR_HANDLE GetAtlasSrv() const { return atlasSrvGPU_; }
    float GetLineHeight() const { return lineHeight_; }
    float GetBaseline() const { return baseline_; }

private:
    Microsoft::WRL::ComPtr<IDWriteFactory> factory_;
    Microsoft::WRL::ComPtr<IDWriteFontCollection> fontCollection_;
    Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace_;

    std::unordered_map<char32_t, Glyph> glyphCache_;

    // アトラス
    Microsoft::WRL::ComPtr<ID3D12Resource> atlasResource_;
    uint32_t atlasSrvIndex_ = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE atlasSrvCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE atlasSrvGPU_{};

    float fontSize_ = 16.0f;
    float lineHeight_ = 0.0f;
    float baseline_ = 0.0f;

    // アトラスのサイズ
    uint32_t atlasWidth_ = 1024;
    uint32_t atlasHeight_ = 1024;
    
    // アトラスのパッキング用
    uint32_t currentX_ = 0;
    uint32_t currentY_ = 0;
    uint32_t currentRowHeight_ = 0;

    // アトラスのピクセルデータ (R8G8B8A8)
    std::vector<uint8_t> atlasPixels_;

    bool CreateAtlasResource();
    bool AddGlyphToAtlas(char32_t codePoint);
    void UpdateAtlasTexture();
};

} // namespace TuboEngine
