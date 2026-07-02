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

/// <summary>
/// DirectWrite を用いてフォントを読み込み、グリフ情報とフォントアトラスを管理するクラス。
/// </summary>
class Font {
public:
    /// <summary>
    /// 文字1個分のグリフ情報（送り幅・オフセット・サイズ・アトラス上のUV）。
    /// </summary>
    struct Glyph {
        float advanceX;
        float offsetX, offsetY;    // bearing
        float width, height;
        Math::Vector2 uvLT, uvRB;  // フォントアトラス上のUV
    };

    /// <summary>
    /// コンストラクタ。
    /// </summary>
    Font() = default;
    /// <summary>
    /// デストラクタ。
    /// </summary>
    ~Font() = default;

    /// <summary>
    /// 初期化処理。
    /// </summary>
    bool Initialize(const std::wstring& filePath, float fontSize);
    /// <summary>
    /// Glyph を取得する。
    /// </summary>
    const Glyph* GetGlyph(char32_t codePoint);

    /// <summary>
    /// AtlasSrv を取得する。
    /// </summary>
    D3D12_GPU_DESCRIPTOR_HANDLE GetAtlasSrv() const { return atlasSrvGPU_; }
    /// <summary>
    /// LineHeight を取得する。
    /// </summary>
    float GetLineHeight() const { return lineHeight_; }
    /// <summary>
    /// Baseline を取得する。
    /// </summary>
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

    /// <summary>
    /// AtlasResource の生成。
    /// </summary>
    bool CreateAtlasResource();
    /// <summary>
    /// GlyphToAtlas を追加する。
    /// </summary>
    bool AddGlyphToAtlas(char32_t codePoint);
    /// <summary>
    /// AtlasTexture の更新。
    /// </summary>
    void UpdateAtlasTexture();
};

} // namespace TuboEngine
