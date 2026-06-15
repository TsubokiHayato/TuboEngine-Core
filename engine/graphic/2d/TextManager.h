#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Font.h"
#include "TextObject.h"
#include "Vector2.h"
#include "Vector4.h"

namespace TuboEngine {

class TextManager {
public:
    static TextManager* GetInstance();
    static void DestroyInstance();

    void Initialize();
    void Finalize();
    void UpdateAll();
    void DrawAll();
    void DrawImGui();

    // よく使うフォント名
    struct PresetFontNames {
        static inline const std::string Best10             = "Best10";
        static inline const std::string SoukouMincho       = "SoukouMincho";
        static inline const std::string UtsukushiFONT      = "UtsukushiFONT";
        static inline const std::string YasashisaGothicBold = "YasashisaGothicBold-V2";
    };

    // テキスト定義（保存用）
    struct TextDefinition {
        std::string name;        // 識別名
        std::string text;        // 表示文字列 (UTF-8)
        std::string fontName;    // 使用フォント名（BaseFont または AddFont の論理名）
        float       fontSize = 32.0f; // 実フォントサイズ
        Math::Vector2 position{0.0f, 0.0f};
        Math::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
        float       scale = 1.0f;     // 見た目のスケール
        // 揃え/アンカー情報
        // 0: Left   1: Center   2: Right
        int         horizontalAlign = 0;
        // 0: Top    1: Middle   2: Bottom
        int         verticalAlign   = 0;
    };

    // フォント管理
    Font* LoadFont(const std::string& name, const std::string& filePath, float size = 32.0f);
    Font* GetFont(const std::string& name);

    Font* LoadFontFromProject(const std::string& name, const std::string& fileName, float size = 32.0f);
    Font* LoadFontFromExternal(const std::string& name, const std::string& fileName, float size = 32.0f);
    Font* LoadFontFromWindows(const std::string& name, const std::string& fileName, float size = 32.0f);

    // ベース名とサイズからフォントを取得/生成（BaseFont/AddFont 用の SizeFont を生成）
    Font* GetOrCreateFontSized(const std::string& baseName, float fontSize);

    // テキスト作成/削除
    TextObject* CreateText(
        const std::string& fontName,
        const std::string& text,
        const Math::Vector2& pos,
        const Math::Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f},
        float scale = 1.0f
    );
    void RemoveText(TextObject* text);

    // 全てのテキストを削除（シーン遷移時などに呼ぶ）
    void ClearAllTexts();

    // JSON レイアウトのロード/セーブ
    bool LoadTextLayout(const std::string& filePath);
    bool SaveTextLayout(const std::string& filePath) const;

private:
    TextManager() = default;
    ~TextManager() = default;
    TextManager(const TextManager&) = delete;
    TextManager& operator=(const TextManager&) = delete;

    static TextManager* instance_;

    // すべての Font インスタンス
    std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;

    // BaseFont: プリセットなどベースにする論理フォント名
    std::vector<std::string> baseFontNames_;
    // AddFont: ImGui の Add Font で追加された論理フォント名
    std::vector<std::string> addedFontNames_;
    // SizeFont: BaseFont/AddFont から生成されたサイズ付きフォント名（"Name_32" など）
    std::vector<std::string> sizedFontNames_;

    std::vector<std::unique_ptr<TextObject>> texts_;

    // 保存用のテキスト定義と TextObject の対応
    std::vector<TextDefinition> textDefs_;

    // TextManager.h（texts_ と同じサイズを保つフラグ）
    std::vector<bool> textAlive_;

    // フォント検索用ディレクトリ
    std::string projectFontDir_ = "Resources/Font/";
    std::string externalFontDir_ = "Resources/Font/external/";
    std::string windowsFontDir_{};

    bool initialized_ = false;
};

} // namespace TuboEngine
