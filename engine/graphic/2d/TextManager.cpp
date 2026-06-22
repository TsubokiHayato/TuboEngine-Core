#include "TextManager.h"
#include "externals/imgui/imgui.h"
#include "ImGuiManager.h" // Window メニューでの表示/非表示制御に PanelPtr を使う
#include <externals/nlohmann/json.hpp>
#include <filesystem> //  ディレクトリ作成用
#include <fstream>
#include <iostream>

namespace TuboEngine {

using nlohmann::json;

//----------------------------------------------------------------------------
// TextManager シングルトン管理
//----------------------------------------------------------------------------

TextManager* TextManager::instance_ = nullptr;

// シングルトンインスタンス取得
// 初回呼び出し時に動的確保し、その後は同じインスタンスを返す。
TextManager* TextManager::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = new TextManager();
    }
    return instance_;
}

// シングルトンインスタンス破棄
// Finalize() を呼び出してリソースを解放した後、インスタンスを delete する。
void TextManager::DestroyInstance() {
    if (instance_) {
        instance_->Finalize();
        delete instance_;
        instance_ = nullptr;
    }
}

//----------------------------------------------------------------------------
// 初期化 / 終了処理
//----------------------------------------------------------------------------

// TextManager 全体の初期化。
// - Windows のフォントディレクトリ設定
// - プリセットフォント(BaseFont)の読み込み
// - BaseFont 名リストの構築
void TextManager::Initialize() {
   if (initialized_) {
        return;
    }
    initialized_ = true;

#ifdef _WIN32
    windowsFontDir_ = "C:/Windows/Fonts/";
#endif

    // プリセット BaseFont 登録
    LoadFontFromExternal(PresetFontNames::Best10, "BestTen-CRT.otf", 32.0f);
    LoadFontFromExternal(PresetFontNames::SoukouMincho, "SoukouMincho.ttf", 32.0f);
    LoadFontFromExternal(PresetFontNames::UtsukushiFONT, "UtsukushiFONT.otf", 24.0f);
    LoadFontFromExternal(PresetFontNames::YasashisaGothicBold, "YasashisaGothicBold-V2.otf", 48.0f);

    baseFontNames_.push_back(PresetFontNames::Best10);
    baseFontNames_.push_back(PresetFontNames::SoukouMincho);
    baseFontNames_.push_back(PresetFontNames::UtsukushiFONT);
    baseFontNames_.push_back(PresetFontNames::YasashisaGothicBold);
}

// TextManager が保持しているフォント／テキスト関連のリソースを全て解放する。
// シングルトン破棄前やシーン終了時などに呼び出す想定。
void TextManager::Finalize() {
    fonts_.clear();
    texts_.clear();
    textDefs_.clear();
    baseFontNames_.clear();
    addedFontNames_.clear();
    sizedFontNames_.clear();
   textAlive_.clear();
    initialized_ = false;
}

//----------------------------------------------------------------------------
// ランタイム更新 / 描画
//----------------------------------------------------------------------------

// 全テキストオブジェクトの Update を実行し、削除予約されたものを一括で削除する。
// ・textAlive_ が false のものは Update をスキップし、ループ後にまとめて erase する。
// ・後ろから erase することでインデックスずれを防いでいる。
void TextManager::UpdateAll() {
    for (size_t i = 0; i < texts_.size(); ++i) {
        if (i < textAlive_.size() && !textAlive_[i])
            continue;
        texts_[i]->Update();
    }

    // ここでフラグが false のものを後ろから erase
    for (size_t i = texts_.size(); i-- > 0;) {
        if (i < textAlive_.size() && !textAlive_[i]) {
            texts_.erase(texts_.begin() + static_cast<std::ptrdiff_t>(i));
            if (i < textDefs_.size()) {
                textDefs_.erase(textDefs_.begin() + static_cast<std::ptrdiff_t>(i));
            }
            textAlive_.erase(textAlive_.begin() + static_cast<std::ptrdiff_t>(i));
        }
    }
}

// 全ての生存テキストオブジェクトを描画する。
// textAlive_ が false のものは描画をスキップする。
void TextManager::DrawAll() {
    for (size_t i = 0; i < texts_.size(); ++i) {
        if (i < textAlive_.size() && !textAlive_[i])
            continue;
        texts_[i]->Draw();
    }
}

//----------------------------------------------------------------------------
// ImGui デバッグ UI
//----------------------------------------------------------------------------

// TextManager 用の ImGui デバッグウィンドウを描画する。
// ・レイアウト(JSON)のロード／セーブ
// ・フォント(AddFont)の追加
// ・ロード済みフォント一覧表示
// ・テキストオブジェクトの生成／編集／削除
void TextManager::DrawImGui() {
#ifdef USE_IMGUI
    if (TuboEngine::ImGuiManager::GetInstance()->BeginPanel("TextManager")) {

    // レイアウトのロード/セーブ
    if (ImGui::CollapsingHeader("Layout")) {
        static char layoutPathBuf[256] = "Resources/Text/Debug.json";
        ImGui::InputText("Layout Path", layoutPathBuf, sizeof(layoutPathBuf));
        if (ImGui::Button("Load Layout")) {
            if (!LoadTextLayout(layoutPathBuf)) {
                std::cerr << "Failed to load text layout: " << layoutPathBuf << std::endl;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Layout")) {
            if (!SaveTextLayout(layoutPathBuf)) {
                std::cerr << "Failed to save text layout: " << layoutPathBuf << std::endl;
            }
        }
    }

    // フォントの追加UI（AddFont）
    // プロジェクト／外部フォルダ／Windows フォルダからフォントを選択してロードし、
    // addedFontNames_ に論理名を登録する。
    if (ImGui::CollapsingHeader("Add Font")) {
        static char fontNameBuf[64] = "";
        static char fontFileBuf[128] = "msgothic.ttc";
        static float fontSize = 32.0f;
        static int sourceType = 2; // 0: Project, 1: External, 2: Windows

        ImGui::InputText("Font Name", fontNameBuf, sizeof(fontNameBuf));
        ImGui::InputText("Font File", fontFileBuf, sizeof(fontFileBuf));
        ImGui::RadioButton("Project", &sourceType, 0);
        ImGui::SameLine();
        ImGui::RadioButton("External", &sourceType, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Windows", &sourceType, 2);
        ImGui::DragFloat("Font Size", &fontSize, 1.0f, 8.0f, 128.0f);

        if (ImGui::Button("Load Font")) {
            if (fontNameBuf[0] != '\0' && fontFileBuf[0] != '\0') {
                Font* font = nullptr;
                std::string fontName(fontNameBuf);
                std::string fileName(fontFileBuf);

                switch (sourceType) {
                case 0:
                    font = LoadFontFromProject(fontName, fileName, fontSize);
                    break;
                case 1:
                    font = LoadFontFromExternal(fontName, fileName, fontSize);
                    break;
                case 2:
                    font = LoadFontFromWindows(fontName, fileName, fontSize);
                    break;
                default:
                    break;
                }

                if (!font) {
                    std::cerr << "Failed to load font via ImGui UI." << std::endl;
                } else {
                    // AddFont グループに登録（BaseFont とは別）
                    addedFontNames_.push_back(fontName);
                }
            }
        }
    }

    // 登録済みフォント一覧（BaseFont / AddFont / SizeFont を区別して表示）
    if (ImGui::CollapsingHeader("Loaded Fonts")) {
        ImGui::Text("BaseFont:");
        for (const std::string& name : baseFontNames_) {
            ImGui::BulletText("%s", name.c_str());
        }
        if (!addedFontNames_.empty()) {
            ImGui::Separator();
            ImGui::Text("AddFont:");
            for (const std::string& name : addedFontNames_) {
                ImGui::BulletText("%s", name.c_str());
            }
        }
        if (!sizedFontNames_.empty()) {
            ImGui::Separator();
            ImGui::Text("SizeFont (generated, not selectable):");
            for (const std::string& name : sizedFontNames_) {
                ImGui::BulletText("%s", name.c_str());
            }
        }
    }

    // テキストオブジェクトの管理UI（定義 + 実体）
    // textDefs_ を編集しながら、対応する TextObject 実体(texts_)に即時反映する。
    if (ImGui::CollapsingHeader("Text Objects")) {
        if (ImGui::Button("Create New Text")) {
            // 新規テキスト定義を追加し、対応する TextObject を生成
            TextDefinition def{};
            def.name = "Text" + std::to_string(textDefs_.size());
            def.text = "New Text";
            def.fontName = PresetFontNames::Best10;
            def.fontSize = 32.0f;
            def.position = Math::Vector2{100.0f, 100.0f};
            def.color = Math::Vector4{1.0f, 1.0f, 1.0f, 1.0f};
            def.scale = 1.0f;
            def.horizontalAlign = 0;
            def.verticalAlign = 0;
            textDefs_.push_back(def);

            Font* font = GetOrCreateFontSized(def.fontName, def.fontSize);
            if (font) {
                // フォントサイズ付き論理名で TextObject を生成
                TextObject* obj = CreateText(def.fontName + "_" + std::to_string(static_cast<int>(def.fontSize)), def.text, def.position, def.color, def.scale);
                if (obj) {
                    obj->SetFont(font);
                    obj->SetHorizontalAlign(def.horizontalAlign);
                    obj->SetVerticalAlign(def.verticalAlign);
                }
            }
        }

        ImGui::Separator();

        // textDefs_ と texts_ は同じインデックスで対応している前提
        for (size_t index = 0; index < textDefs_.size() && index < texts_.size();) {
            TextDefinition& def = textDefs_[index];
            TextObject* textObj = texts_[index].get();

            ImGui::PushID(static_cast<int>(index));
            // ツリーラベル: 表示名 + 固定ID + index（ImGui の ID は ## 以降）
            char treeLabel[128];
            std::snprintf(treeLabel, sizeof(treeLabel), "TextObject [%s]##TextDef%zu", def.name.c_str(), index);
            bool isOpen = ImGui::TreeNode(treeLabel);
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                // 論理削除: フラグを false にする
                // 実際の erase は UpdateAll() の後ろから走査で行う。
                if (index < textAlive_.size()) {
                    textAlive_[index] = false;
                }
                ImGui::PopID();
                if (isOpen) {
                    ImGui::TreePop();
                }
                ++index;
                continue;
            }

            if (isOpen) {
                // name
                char nameBuf[64];
                std::snprintf(nameBuf, sizeof(nameBuf), "%s", def.name.c_str());
                if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                    def.name = nameBuf;
                }

                // text: use a local fixed buffer each frame, seeded from def.text
                // ImGui の編集結果を TextObject に即時反映する。
                {
                    char textBuf[1024];
                    std::size_t len = def.text.size();
                    if (len >= sizeof(textBuf)) {
                        len = sizeof(textBuf) - 1;
                    }
                    std::memcpy(textBuf, def.text.data(), len);
                    textBuf[len] = '\0';
                    if (ImGui::InputTextMultiline("Text", textBuf, sizeof(textBuf))) {
                        def.text = textBuf;
                        textObj->SetText(def.text);
                    }
                }

                // position
                float pos[2] = {def.position.x, def.position.y};
                if (ImGui::DragFloat2("Position", pos, 1.0f)) {
                    def.position.x = pos[0];
                    def.position.y = pos[1];
                    textObj->SetPosition(def.position);
                }

                // color
                float col[4] = {def.color.x, def.color.y, def.color.z, def.color.w};
                if (ImGui::ColorEdit4("Color", col)) {
                    def.color = Math::Vector4{col[0], col[1], col[2], col[3]};
                    textObj->SetColor(def.color);
                }

                // scale（見た目のスケール調整）
                float scale = def.scale;
                if (ImGui::DragFloat("Scale", &scale, 0.01f, 0.1f, 10.0f)) {
                    def.scale = scale;
                    textObj->SetScale(def.scale);
                }

                // 揃え設定
                {
                    static const char* hItems[] = { "Left", "Center", "Right" };
                    int hAlign = def.horizontalAlign;
                    if (ImGui::Combo("Horizontal Align", &hAlign, hItems, IM_ARRAYSIZE(hItems))) {
                        def.horizontalAlign = hAlign;
                        textObj->SetHorizontalAlign(def.horizontalAlign);
                    }

                    static const char* vItems[] = { "Top", "Middle", "Bottom" };
                    int vAlign = def.verticalAlign;
                    if (ImGui::Combo("Vertical Align", &vAlign, vItems, IM_ARRAYSIZE(vItems))) {
                        def.verticalAlign = vAlign;
                        textObj->SetVerticalAlign(def.verticalAlign);
                    }
                }

                // BaseFont + AddFont から選択（SizeFont は対象外）
                // selectableFontNames から選択した baseName と fontSize から
                // GetOrCreateFontSized() を通して実フォントを取得する。
                std::vector<const char*> selectableFontNames;
                std::vector<std::string> selectableNamesStorage;
                // BaseFont
                for (const std::string& n : baseFontNames_) {
                    selectableNamesStorage.push_back(n);
                }
                // AddFont
                for (const std::string& n : addedFontNames_) {
                    selectableNamesStorage.push_back(n);
                }
                for (std::string& s : selectableNamesStorage) {
                    selectableFontNames.push_back(s.c_str());
                }

                int baseIndex = 0;
                for (int i = 0; i < static_cast<int>(selectableNamesStorage.size()); ++i) {
                    if (def.fontName == selectableNamesStorage[i]) {
                        baseIndex = i;
                        break;
                    }
                }
                if (!selectableFontNames.empty()) {
                    if (ImGui::Combo("Base/Add Font", &baseIndex, selectableFontNames.data(), static_cast<int>(selectableFontNames.size()))) {
                        def.fontName = selectableNamesStorage[baseIndex];
                        Font* sizedFont = GetOrCreateFontSized(def.fontName, def.fontSize);
                        if (sizedFont) {
                            textObj->SetFont(sizedFont);
                        }
                    }
                }

                // フォントサイズ（実サイズ）
                // baseName とサイズから SizeFont を生成し、そのフォントを TextObject に設定する。
                float fontSize = def.fontSize;
                if (ImGui::DragFloat("Font Size", &fontSize, 1.0f, 8.0f, 128.0f)) {
                    def.fontSize = fontSize;
                    Font* sizedFont = GetOrCreateFontSized(def.fontName, def.fontSize);
                    if (sizedFont) {
                        textObj->SetFont(sizedFont);
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
            ++index;
        }
    }

    }
    TuboEngine::ImGuiManager::GetInstance()->EndPanel();
#endif
}

//----------------------------------------------------------------------------
// フォント読み込み関連
//----------------------------------------------------------------------------

// 実際のファイルパスとサイズを指定してフォントを読み込む共通関数。
// - すでに同名のフォントが存在する場合はそれを再利用。
// - 成功時は Font* を返し、失敗時は nullptr を返す。
Font* TextManager::LoadFont(const std::string& name, const std::string& filePath, float size) {
    std::unordered_map<std::string, std::unique_ptr<Font>>::iterator it = fonts_.find(name);
    if (it != fonts_.end()) {
        return it->second.get();
    }

    std::unique_ptr<Font> font = std::make_unique<Font>();
    std::wstring filePathW(filePath.begin(), filePath.end());
    if (font->Initialize(filePathW, size)) {
        Font* ptr = font.get();
        fonts_[name] = std::move(font);
        return ptr;
    }

    std::cerr << "Failed to load font: " << filePath << std::endl;
    return nullptr;
}

// プロジェクト内フォントディレクトリからフォントを読み込む。
Font* TextManager::LoadFontFromProject(const std::string& name, const std::string& fileName, float size) {
    std::string path = projectFontDir_ + fileName;
    return LoadFont(name, path, size);
}

// 外部フォントディレクトリからフォントを読み込む。
Font* TextManager::LoadFontFromExternal(const std::string& name, const std::string& fileName, float size) {
    std::string path = externalFontDir_ + fileName;
    return LoadFont(name, path, size);
}

// Windows 標準フォントディレクトリからフォントを読み込む。
// 非 Windows 環境ではダミー実装となる。
Font* TextManager::LoadFontFromWindows(const std::string& name, const std::string& fileName, float size) {
#ifdef _WIN32
    std::string path = windowsFontDir_ + fileName;
    return LoadFont(name, path, size);
#else
    (void)name;
    (void)fileName;
    (void)size;
    std::cerr << "LoadFontFromWindows is only supported on Windows." << std::endl;
    return nullptr;
#endif
}

// 名前から Font インスタンスを取得するヘルパー。
// 未ロードの場合は nullptr を返す。
Font* TextManager::GetFont(const std::string& name) {
    std::unordered_map<std::string, std::unique_ptr<Font>>::iterator it = fonts_.find(name);
    if (it != fonts_.end()) {
        return it->second.get();
    }
    return nullptr;
}

// BaseFont / AddFont とフォントサイズから、サイズ付きフォント(SizeFont)を取得もしくは生成する。
// 論理名は「baseName_サイズ」（例: "Best10_32"）とし、すでに存在する場合は再利用する。
Font* TextManager::GetOrCreateFontSized(const std::string& baseName, float fontSize) {
    std::string sizedName = baseName + "_" + std::to_string(static_cast<int>(fontSize));

    Font* existing = GetFont(sizedName);
    if (existing) {
        return existing;
    }

    // BaseFont の場合は固定のファイル名にマッピング。
    // AddFont からの baseName は、そのまま external フォントファイル名とみなす。
    std::string fileName;
    if (baseName == PresetFontNames::Best10) {
        fileName = "BestTen-CRT.otf";
    } else if (baseName == PresetFontNames::SoukouMincho) {
        fileName = "SoukouMincho.ttf";
    } else if (baseName == PresetFontNames::UtsukushiFONT) {
        fileName = "UtsukushiFONT.otf";
    } else if (baseName == PresetFontNames::YasashisaGothicBold) {
        fileName = "YasashisaGothicBold-V2.otf";
    } else {
        // AddFont からの baseName は、そのまま external ファイル名とみなす
        fileName = baseName;
    }

    Font* sized = LoadFontFromExternal(sizedName, fileName, fontSize);
    if (sized) {
        sizedFontNames_.push_back(sizedName);
    }
    return sized;
}

//----------------------------------------------------------------------------
// TextObject 生成 / 削除
//----------------------------------------------------------------------------

// テキストオブジェクトを生成する。
// - 指定フォント名から Font を取得できなければ nullptr を返す。
// - 生成した TextObject は TextManager 内部の texts_ に所有させ、textAlive_ に true を追加する。
TextObject* TextManager::CreateText(const std::string& fontName, const std::string& text, const Math::Vector2& pos, const Math::Vector4& color, float scale) {
    Font* font = GetFont(fontName);
    if (!font) {
        std::cerr << "Font not found: " << fontName << std::endl;
        return nullptr;
    }

    std::unique_ptr<TextObject> textObj = std::make_unique<TextObject>();
    textObj->Initialize();
    textObj->SetFont(font);
    textObj->SetText(text);
    textObj->SetPosition(pos);
    textObj->SetColor(color);
    textObj->SetScale(scale);

    TextObject* ptr = textObj.get();
    texts_.push_back(std::move(textObj));
    textAlive_.push_back(true); 
    return ptr;
}

// TextObject を削除予約する。
// 実際の削除は UpdateAll() で行われるため、ここでは textAlive_ のフラグを false にするだけ。
void TextManager::RemoveText(TextObject* text) {
    auto it = std::find_if(texts_.begin(), texts_.end(), [text](const std::unique_ptr<TextObject>& ptr) { return ptr.get() == text; });
    if (it != texts_.end()) {
        size_t idx = static_cast<size_t>(std::distance(texts_.begin(), it));
        if (idx < textAlive_.size()) {
            textAlive_[idx] = false; // 削除予約
        }
    }
}

// すべてのテキスト（及びレイアウト定義）を削除
// シーン終了時などに残らないようにするため呼び出します。
void TextManager::ClearAllTexts() {
    textDefs_.clear();
    texts_.clear();
    textAlive_.clear();
}

//----------------------------------------------------------------------------
// レイアウト(JSON)ロード/セーブ
//----------------------------------------------------------------------------

// テキストレイアウト(JSON)を読み込み、TextManager 内部の textDefs_ および texts_ を再構築する。
// - 事前に DirectX のコマンドを一通り実行し、GPU の処理完了を待ってから破棄・再生成を行う。
// - "texts" 配列から TextDefinition を構築し、それに対応する TextObject を CreateText で生成する。
bool TextManager::LoadTextLayout(const std::string& filePath) {
    // レイアウト差し替え前に一度 GPU 完了を待つ
    {
        auto* dx = TuboEngine::DirectXCommon::GetInstance();
        dx->CommandExecution(); // 何もコマンドを積んでいなければ fence 待ちになるだけ
    }

    std::ifstream ifs(filePath);
    if (!ifs) {
        return false;
    }

    json root;
    try {
        ifs >> root;
    } catch (...) {
        std::cerr << "Failed to parse text layout JSON: " << filePath << std::endl;
        return false;
    }

    if (!root.contains("texts") || !root["texts"].is_array()) {
        return false;
    }

    // 既存の定義・実体をクリアしてから、新しいレイアウトを構築する。
    textDefs_.clear();
    texts_.clear();
    textAlive_.clear();

    // 以下は今の実装のまま
    for (json& jt : root["texts"]) {
        TextDefinition def{};
        def.name = jt.value("name", "");
        def.text = jt.value("text", "");
        def.fontName = jt.value("font", PresetFontNames::Best10);
        def.fontSize = jt.value("fontSize", 32.0f);
        std::vector<float> posArr = jt.value("position", std::vector<float>{0.0f, 0.0f});
        std::vector<float> colArr = jt.value("color", std::vector<float>{1.0f, 1.0f, 1.0f, 1.0f});
        def.scale = jt.value("scale", 1.0f);
        def.horizontalAlign = jt.value("horizontalAlign", 0);
        def.verticalAlign   = jt.value("verticalAlign", 0);

        if (posArr.size() >= 2) {
            def.position = Math::Vector2{posArr[0], posArr[1]};
        }
        if (colArr.size() >= 4) {
            def.color = Math::Vector4{colArr[0], colArr[1], colArr[2], colArr[3]};
        }

        textDefs_.push_back(def);
    }

    // 読み込んだ定義から TextObject 実体を生成し、フォントを設定する。
    for (TextDefinition& def : textDefs_) {
        Font* font = GetOrCreateFontSized(def.fontName, def.fontSize);
        if (font) {
            TextObject* obj = CreateText(def.fontName + "_" + std::to_string(static_cast<int>(def.fontSize)), def.text, def.position, def.color, def.scale);
            if (obj) {
                obj->SetFont(font);
                obj->SetHorizontalAlign(def.horizontalAlign);
                obj->SetVerticalAlign(def.verticalAlign);
            }
        }
    }

    return true;
}

// 現在の textDefs_ を JSON 形式でファイルに書き出す。
// - "texts" 配列に各 TextDefinition を保存。
// - 必要であれば出力先ディレクトリを自動生成する。
bool TextManager::SaveTextLayout(const std::string& filePath) const {
    json root;
    root["texts"] = json::array();

    for (const TextDefinition& def : textDefs_) {
        json jt;
        jt["name"] = def.name;
        jt["text"] = def.text;
        jt["font"] = def.fontName;
        jt["fontSize"] = def.fontSize;
        jt["position"] = {def.position.x, def.position.y};
        jt["color"] = {def.color.x, def.color.y, def.color.z, def.color.w};
        jt["scale"] = def.scale;
        jt["horizontalAlign"] = def.horizontalAlign;
        jt["verticalAlign"]   = def.verticalAlign;
        root["texts"].push_back(jt);
    }

    // 親ディレクトリが無ければ作成
    try {
        std::filesystem::path path(filePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directories for layout: " << e.what() << std::endl;
        return false;
    }

    std::ofstream ofs(filePath);
    if (!ofs) {
        std::cerr << "Failed to open layout file for writing: " << filePath << std::endl;
        return false;
    }
    ofs << root.dump(2);
    return true;
}

} // namespace TuboEngine