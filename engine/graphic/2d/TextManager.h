#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Font.h"
#include "TextObject.h"
#include "Sprite.h"
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

    // 画面上の置き場所プリセット（テキスト/スプライトを上下左右に楽に配置するための機能）。
    // None 以外を指定すると、margin を画面端からの余白として位置を自動計算し、
    // 揃え(HorizontalAlign/VerticalAlign)も対応する向きへ自動設定する。
    enum class ScreenAnchor {
        None = 0,
        TopLeft, TopCenter, TopRight,
        MiddleLeft, Center, MiddleRight,
        BottomLeft, BottomCenter, BottomRight,
    };

    // 他のテキスト/スプライト（name で指定）の近くに、縦・横方向で楽に配置するための機能。
    // 例: ある画像の上にラベルを置く、ボタンの右にテキストを置く、など。
    enum class RelativeAnchor {
        None = 0,
        Above,   // 基準の上（縦方向）。横方向は基準の中央に揃う
        Below,   // 基準の下（縦方向）。横方向は基準の中央に揃う
        LeftOf,  // 基準の左（横方向）。縦方向は基準の中央に揃う
        RightOf, // 基準の右（横方向）。縦方向は基準の中央に揃う
    };

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

        // 画面端アンカー（None なら position を直接使う。それ以外なら毎フレーム
        // 画面サイズと margin から position/Align を自動計算して上書きする）
        ScreenAnchor anchor = ScreenAnchor::None;
        Math::Vector2 margin{20.0f, 20.0f};

        // 相対アンカー（他のテキスト/スプライトの name を基準に、その上下左右へ配置する）
        std::string relativeTo;      // 基準オブジェクトの name（空なら無効）
        RelativeAnchor relativeAnchor = RelativeAnchor::None;
        float relativeGap = 8.0f;    // 基準との隙間(px)
    };

    // スプライト定義（保存用）
    struct SpriteDefinition {
        std::string name;                 // 識別名
        std::string textureFilePath;      // テクスチャパス（"Resources/Textures/" からの相対）
        Math::Vector2 position{0.0f, 0.0f};
        Math::Vector2 size{160.0f, 160.0f};
        Math::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
        float       rotation = 0.0f;

        // ピボット（Sprite::SetAnchorPoint。0~1 の正規化座標。{0,0}=左上 {0.5,0.5}=中央 {1,1}=右下）。
        // ScreenAnchor/RelativeAnchor を使わず手動で位置決めしたい場合はここを直接編集する。
        Math::Vector2 anchorPoint{0.0f, 0.0f};

        // 画面端アンカー（テキストと同様。Sprite::SetAnchorPoint を使って端からの余白を表現する）
        ScreenAnchor anchor = ScreenAnchor::None;
        Math::Vector2 margin{20.0f, 20.0f};

        // 相対アンカー（他のテキスト/スプライトの name を基準に、その上下左右へ配置する）
        std::string relativeTo;
        RelativeAnchor relativeAnchor = RelativeAnchor::None;
        float relativeGap = 8.0f;
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

    // 画面端アンカー指定でテキストを作成する便利関数。
    // margin は画面端からの余白(px)。anchor に応じて HorizontalAlign/VerticalAlign も自動設定する。
    TextObject* CreateTextAtAnchor(
        const std::string& fontName,
        const std::string& text,
        ScreenAnchor anchor,
        const Math::Vector2& margin = {20.0f, 20.0f},
        const Math::Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f},
        float scale = 1.0f
    );

    // 全てのテキストを削除（シーン遷移時などに呼ぶ）
    void ClearAllTexts();

    // スプライト作成/削除
    Sprite* CreateSprite(
        const std::string& textureFilePath,
        const Math::Vector2& pos,
        const Math::Vector2& size = {160.0f, 160.0f},
        const Math::Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f}
    );
    void RemoveSprite(Sprite* sprite);

    // 画面端アンカー指定でスプライトを作成する便利関数。
    Sprite* CreateSpriteAtAnchor(
        const std::string& textureFilePath,
        ScreenAnchor anchor,
        const Math::Vector2& size = {160.0f, 160.0f},
        const Math::Vector2& margin = {20.0f, 20.0f},
        const Math::Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f}
    );

    // 全てのスプライトを削除（シーン遷移時などに呼ぶ）
    void ClearAllSprites();

    // JSON レイアウトのロード/セーブ
    bool LoadTextLayout(const std::string& filePath);
    bool SaveTextLayout(const std::string& filePath) const;

private:
    // ScreenAnchor + margin から「位置」と「揃え(h/v Align)」を計算するヘルパー。
    // out 引数で結果を返す（TextObject 用）。
    void ComputeAnchorPlacement(ScreenAnchor anchor, const Math::Vector2& margin, Math::Vector2& outPos, int& outHAlign, int& outVAlign) const;

    // テキスト1件にアンカーを反映する（位置 + 揃え）。
    void ApplyAnchorToText(TextDefinition& def, TextObject* obj) const;
    // スプライト1件にアンカーを反映する（位置はテクスチャ左上基準。右/下端は size を引いて合わせる）。
    void ApplyAnchorToSprite(SpriteDefinition& def, Sprite* obj) const;

    // name で指定したテキスト/スプライトの矩形（左上座標 + サイズ）を取得する。
    // テキスト/スプライトのどちらも検索対象。見つからなければ false。
    bool TryGetBoundsByName(const std::string& name, Math::Vector2& outTopLeft, Math::Vector2& outSize) const;

    // 相対アンカー（他オブジェクトの上下左右）をテキスト/スプライト1件に反映する。
    void ApplyRelativeAnchorToText(TextDefinition& def, TextObject* obj) const;
    void ApplyRelativeAnchorToSprite(SpriteDefinition& def, Sprite* obj) const;

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

    std::vector<std::unique_ptr<Sprite>> sprites_;
    // 保存用のスプライト定義と Sprite の対応
    std::vector<SpriteDefinition> spriteDefs_;
    // sprites_ と同じサイズを保つフラグ
    std::vector<bool> spriteAlive_;

    // フォント検索用ディレクトリ
    std::string projectFontDir_ = "Resources/Font/";
    std::string externalFontDir_ = "Resources/Font/external/";
    std::string windowsFontDir_{};

    bool initialized_ = false;
};

} // namespace TuboEngine
