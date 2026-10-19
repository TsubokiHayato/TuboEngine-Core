#include "EditorPanels.h"
#include "Logger.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h" // RenderTextEllipsis
#endif // USE_IMGUI

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#ifdef USE_IMGUI
namespace fs = std::filesystem;

namespace {

///-----------------------------------------------------------------------
///                             共通
///-----------------------------------------------------------------------

// path → UTF-8 文字列（ImGui は UTF-8 で表示する）
std::string ToUtf8(const fs::path& path) {
	const std::u8string u8 = path.generic_u8string();
	return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

// 大文字小文字を無視した部分一致
bool ContainsIgnoreCase(const std::string& text, const std::string& word) {
	auto it = std::search(text.begin(), text.end(), word.begin(), word.end(), [](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
	});
	return it != text.end();
}

///-----------------------------------------------------------------------
///                             Project
///-----------------------------------------------------------------------

// Project ウィンドウで表示するルートフォルダ（作業ディレクトリからの相対）
const fs::path kProjectRoot = "Resources";

// フォルダツリーのノード
struct DirNode {
	fs::path path;
	std::string name;
	std::vector<DirNode> children;
};

// 右側のアイコン一覧の1項目
struct Entry {
	fs::path path;
	std::string name;
	std::string ext; // 小文字・ドット無し
	bool isDirectory = false;
};

struct ProjectState {
	bool loaded = false;
	DirNode root;
	fs::path currentDir = kProjectRoot;
	fs::path cachedDir;
	std::vector<Entry> entries;
	fs::path selected;
	char search[128] = "";
	std::string cachedSearch;
	std::vector<Entry> searchResults;
} gProject;

void BuildTree(DirNode& node) {
	std::error_code ec;
	for (const auto& item : fs::directory_iterator(node.path, ec)) {
		if (item.is_directory(ec)) {
			DirNode child;
			child.path = item.path();
			child.name = ToUtf8(item.path().filename());
			BuildTree(child);
			node.children.push_back(std::move(child));
		}
	}
	std::sort(node.children.begin(), node.children.end(), [](const DirNode& a, const DirNode& b) { return a.name < b.name; });
}

Entry MakeEntry(const fs::directory_entry& item) {
	std::error_code ec;
	Entry e;
	e.path = item.path();
	e.name = ToUtf8(item.path().filename());
	e.isDirectory = item.is_directory(ec);
	e.ext = ToUtf8(item.path().extension());
	if (!e.ext.empty() && e.ext[0] == '.') {
		e.ext.erase(0, 1);
	}
	std::transform(e.ext.begin(), e.ext.end(), e.ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return e;
}

// フォルダが先、その後ファイル（それぞれ名前順）
void SortEntries(std::vector<Entry>& entries) {
	std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
		if (a.isDirectory != b.isDirectory) {
			return a.isDirectory;
		}
		return a.name < b.name;
	});
}

void LoadEntries(const fs::path& dir) {
	gProject.entries.clear();
	std::error_code ec;
	for (const auto& item : fs::directory_iterator(dir, ec)) {
		gProject.entries.push_back(MakeEntry(item));
	}
	SortEntries(gProject.entries);
	gProject.cachedDir = dir;
}

// Resources 以下を再帰検索（件数上限あり）
void SearchEntries(const std::string& word) {
	constexpr size_t kMaxResults = 500;
	gProject.searchResults.clear();
	std::error_code ec;
	for (auto it = fs::recursive_directory_iterator(kProjectRoot, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
		if (ec) {
			break;
		}
		Entry e = MakeEntry(*it);
		if (ContainsIgnoreCase(e.name, word)) {
			gProject.searchResults.push_back(std::move(e));
			if (gProject.searchResults.size() >= kMaxResults) {
				break;
			}
		}
	}
	SortEntries(gProject.searchResults);
	gProject.cachedSearch = word;
}

void Reload() {
	gProject.root = DirNode{kProjectRoot, ToUtf8(kProjectRoot), {}};
	BuildTree(gProject.root);
	std::error_code ec;
	if (!fs::is_directory(gProject.currentDir, ec)) {
		gProject.currentDir = kProjectRoot;
	}
	LoadEntries(gProject.currentDir);
	gProject.cachedSearch.clear();
	gProject.searchResults.clear();
	gProject.loaded = true;
}

// 拡張子ごとのアイコン色と表示ラベル
ImU32 IconColor(const Entry& e) {
	const std::string& x = e.ext;
	if (x == "png" || x == "jpg" || x == "jpeg" || x == "dds" || x == "bmp" || x == "tga") return IM_COL32(80, 160, 90, 255);   // テクスチャ
	if (x == "obj" || x == "gltf" || x == "glb" || x == "fbx" || x == "mtl") return IM_COL32(70, 130, 200, 255);               // モデル
	if (x == "wav" || x == "mp3" || x == "ogg") return IM_COL32(210, 140, 50, 255);                                            // オーディオ
	if (x == "hlsl" || x == "hlsli") return IM_COL32(150, 100, 200, 255);                                                      // シェーダ
	if (x == "json" || x == "csv" || x == "txt") return IM_COL32(170, 170, 110, 255);                                          // データ
	if (x == "ttf" || x == "otf" || x == "ttc") return IM_COL32(190, 90, 90, 255);                                             // フォント
	return IM_COL32(110, 110, 110, 255);
}

void DrawEntryIcon(ImDrawList* dl, const Entry& e, ImVec2 min, float size) {
	if (e.isDirectory) {
		// フォルダ（タブ付きの箱）
		const ImU32 col = IM_COL32(200, 170, 90, 255);
		dl->AddRectFilled(ImVec2(min.x + size * 0.08f, min.y + size * 0.18f), ImVec2(min.x + size * 0.45f, min.y + size * 0.30f), col, 2.0f);
		dl->AddRectFilled(ImVec2(min.x + size * 0.08f, min.y + size * 0.26f), ImVec2(min.x + size * 0.92f, min.y + size * 0.82f), col, 3.0f);
		return;
	}
	// ファイル（色付きの紙＋拡張子）
	const ImVec2 a(min.x + size * 0.18f, min.y + size * 0.10f);
	const ImVec2 b(min.x + size * 0.82f, min.y + size * 0.90f);
	dl->AddRectFilled(a, b, IconColor(e), 3.0f);
	std::string label = e.ext;
	std::transform(label.begin(), label.end(), label.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	if (label.size() > 4) {
		label.resize(4);
	}
	const ImVec2 ts = ImGui::CalcTextSize(label.c_str());
	dl->AddText(ImVec2((a.x + b.x - ts.x) * 0.5f, (a.y + b.y - ts.y) * 0.5f), IM_COL32(255, 255, 255, 230), label.c_str());
}

// エクスプローラーでファイルを選択した状態で開く
void ShowInExplorer(const fs::path& path) {
	std::error_code ec;
	const std::wstring abs = fs::absolute(path, ec).make_preferred().wstring();
	const std::wstring args = L"/select,\"" + abs + L"\"";
	ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
}

void DrawDirTree(const DirNode& node, bool isRoot) {
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (node.children.empty()) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	if (isRoot) {
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	if (gProject.currentDir == node.path) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	const bool opened = ImGui::TreeNodeEx(node.name.c_str(), flags);
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		gProject.currentDir = node.path;
		gProject.search[0] = '\0';
	}
	if (opened) {
		for (const DirNode& child : node.children) {
			DrawDirTree(child, false);
		}
		ImGui::TreePop();
	}
}

void DrawEntryGrid(const std::vector<Entry>& entries) {
	const float iconSize = 56.0f;
	const float cellW = 84.0f;
	const float cellH = iconSize + ImGui::GetTextLineHeight() + 8.0f;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const int columns = (std::max)(1, static_cast<int>((ImGui::GetContentRegionAvail().x + spacing) / (cellW + spacing)));
	ImDrawList* dl = ImGui::GetWindowDrawList();

	fs::path navigateTo;
	for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
		const Entry& e = entries[i];
		if (i % columns != 0) {
			ImGui::SameLine();
		}
		ImGui::PushID(i);
		const ImVec2 p = ImGui::GetCursorScreenPos();
		const bool selected = (gProject.selected == e.path);
		if (ImGui::Selectable("##entry", selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(cellW, cellH))) {
			gProject.selected = e.path;
			if (e.isDirectory && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				navigateTo = e.path;
			}
		}
		// ドラッグ＆ドロップ（ペイロード "ASSET_PATH" = Resources からの相対パス文字列）
		if (ImGui::BeginDragDropSource()) {
			const std::string rel = ToUtf8(e.path);
			ImGui::SetDragDropPayload("ASSET_PATH", rel.c_str(), rel.size() + 1);
			ImGui::TextUnformatted(rel.c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
			ImGui::SetTooltip("%s", ToUtf8(e.path).c_str());
		}
		// 右クリックメニュー
		if (ImGui::BeginPopupContextItem("##ctx")) {
			gProject.selected = e.path;
			if (ImGui::MenuItem("パスをコピー")) {
				ImGui::SetClipboardText(ToUtf8(e.path).c_str());
			}
			if (ImGui::MenuItem("ファイル名をコピー")) {
				ImGui::SetClipboardText(e.name.c_str());
			}
			if (ImGui::MenuItem("エクスプローラーで表示")) {
				ShowInExplorer(e.path);
			}
			ImGui::EndPopup();
		}

		DrawEntryIcon(dl, e, ImVec2(p.x + (cellW - iconSize) * 0.5f, p.y + 2.0f), iconSize);
		// 名前（はみ出す分は … で省略）
		const ImVec2 textMin(p.x + 2.0f, p.y + iconSize + 4.0f);
		const ImVec2 textMax(p.x + cellW - 2.0f, textMin.y + ImGui::GetTextLineHeight());
		const float textW = ImGui::CalcTextSize(e.name.c_str()).x;
		const ImVec2 textPos(textW < cellW - 4.0f ? p.x + (cellW - textW) * 0.5f : textMin.x, textMin.y);
		ImGui::RenderTextEllipsis(dl, textPos, textMax, textMax.x, textMax.x, e.name.c_str(), nullptr, nullptr);
		ImGui::PopID();
	}
	if (!navigateTo.empty()) {
		gProject.currentDir = navigateTo;
	}
}

///-----------------------------------------------------------------------
///                             Console
///-----------------------------------------------------------------------

enum class LogLevel { Log, Warning, Error };

struct ConsoleLine {
	std::string text;
	LogLevel level = LogLevel::Log;
};

struct ConsoleState {
	std::vector<ConsoleLine> lines;
	size_t revision = static_cast<size_t>(-1);
	bool autoScroll = true;
	bool showLog = true;
	bool showWarning = true;
	bool showError = true;
	char search[128] = "";
	int counts[3] = {};
} gConsole;

// 文言からログの種類を推定する（Logger は種類を持たないため）
LogLevel Classify(const std::string& text) {
	if (ContainsIgnoreCase(text, "error") || ContainsIgnoreCase(text, "failed") || text.find("エラー") != std::string::npos || text.find("失敗") != std::string::npos) {
		return LogLevel::Error;
	}
	if (ContainsIgnoreCase(text, "warning") || ContainsIgnoreCase(text, "warn") || text.find("警告") != std::string::npos) {
		return LogLevel::Warning;
	}
	return LogLevel::Log;
}

void RefreshConsole() {
	const size_t revision = TuboEngine::Logger::GetRevision();
	if (revision == gConsole.revision) {
		return;
	}
	gConsole.revision = revision;
	gConsole.lines.clear();
	gConsole.counts[0] = gConsole.counts[1] = gConsole.counts[2] = 0;
	for (std::string& text : TuboEngine::Logger::GetHistory()) {
		ConsoleLine line;
		line.level = Classify(text);
		line.text = std::move(text);
		++gConsole.counts[static_cast<int>(line.level)];
		gConsole.lines.push_back(std::move(line));
	}
}

// 種類の表示切り替えボタン（Unity の右上の Log/Warning/Error トグル風）
void LevelToggle(const char* label, int count, bool* value, const ImVec4& color) {
	char buf[64];
	snprintf(buf, sizeof(buf), "%s %d", label, count);
	ImGui::PushStyleColor(ImGuiCol_Button, *value ? ImVec4(0.30f, 0.30f, 0.30f, 1.0f) : ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, *value ? color : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	if (ImGui::SmallButton(buf)) {
		*value = !*value;
	}
	ImGui::PopStyleColor(2);
}

} // namespace
#endif // USE_IMGUI

namespace TuboEngine::Editor {

void DrawProjectWindow(bool* open) {
#ifdef USE_IMGUI
	if (!ImGui::Begin("Project", open)) {
		ImGui::End();
		return;
	}
	if (!gProject.loaded) {
		Reload();
	}

	std::error_code ec;
	if (!fs::is_directory(kProjectRoot, ec)) {
		ImGui::TextDisabled("Resources フォルダが見つかりません（作業ディレクトリ: %s）", ToUtf8(fs::current_path(ec)).c_str());
		ImGui::End();
		return;
	}

	// ツールバー：更新 / パンくず / 検索
	const float rightEdge = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
	if (ImGui::SmallButton("更新")) {
		Reload();
	}
	ImGui::SameLine();
	{
		// Resources からのパスをボタンで表示し、クリックでそのフォルダへ移動
		fs::path accumulated;
		int index = 0;
		for (const fs::path& part : gProject.currentDir) {
			accumulated /= part;
			if (index > 0) {
				ImGui::SameLine(0.0f, 2.0f);
				ImGui::TextDisabled(">");
				ImGui::SameLine(0.0f, 2.0f);
			}
			ImGui::PushID(index++);
			if (ImGui::SmallButton(ToUtf8(part).c_str())) {
				gProject.currentDir = accumulated;
				gProject.search[0] = '\0';
			}
			ImGui::PopID();
		}
	}
	const float searchWidth = 180.0f;
	ImGui::SameLine((std::max)(ImGui::GetCursorPosX() + 8.0f, rightEdge - searchWidth));
	ImGui::SetNextItemWidth(searchWidth);
	ImGui::InputTextWithHint("##search", "検索 (Resources 全体)", gProject.search, IM_ARRAYSIZE(gProject.search));

	// 左：フォルダツリー / 右：中身
	const float statusHeight = ImGui::GetTextLineHeightWithSpacing() + 4.0f;
	if (ImGui::BeginTable("##project", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV, ImVec2(0.0f, -statusHeight))) {
		ImGui::TableSetupColumn("tree", ImGuiTableColumnFlags_WidthFixed, 200.0f);
		ImGui::TableSetupColumn("items", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		if (ImGui::BeginChild("##tree")) {
			DrawDirTree(gProject.root, true);
		}
		ImGui::EndChild();

		ImGui::TableSetColumnIndex(1);
		if (ImGui::BeginChild("##items")) {
			const std::string word = gProject.search;
			if (!word.empty()) {
				if (word != gProject.cachedSearch) {
					SearchEntries(word);
				}
				if (gProject.searchResults.empty()) {
					ImGui::TextDisabled("「%s」に一致するファイルはありません", word.c_str());
				}
				DrawEntryGrid(gProject.searchResults);
			} else {
				if (gProject.cachedDir != gProject.currentDir) {
					LoadEntries(gProject.currentDir);
				}
				if (gProject.entries.empty()) {
					ImGui::TextDisabled("(空のフォルダ)");
				}
				DrawEntryGrid(gProject.entries);
			}
		}
		ImGui::EndChild();
		ImGui::EndTable();
	}

	// ステータスバー：選択中のパス
	ImGui::Separator();
	if (gProject.selected.empty()) {
		ImGui::TextDisabled("右クリックでパスをコピーできます");
	} else {
		const std::string rel = ToUtf8(gProject.selected);
		ImGui::TextUnformatted(rel.c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("コピー")) {
			ImGui::SetClipboardText(rel.c_str());
		}
	}
	ImGui::End();
#else
	(void)open;
#endif // USE_IMGUI
}

void DrawConsoleWindow(bool* open) {
#ifdef USE_IMGUI
	if (!ImGui::Begin("Console", open)) {
		ImGui::End();
		return;
	}
	RefreshConsole();

	// ツールバー
	const float rightEdge = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
	if (ImGui::SmallButton("クリア")) {
		TuboEngine::Logger::ClearHistory();
		RefreshConsole();
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("全てコピー")) {
		std::string all;
		for (const ConsoleLine& line : gConsole.lines) {
			all += line.text;
			all += '\n';
		}
		ImGui::SetClipboardText(all.c_str());
	}
	ImGui::SameLine();
	ImGui::Checkbox("自動スクロール", &gConsole.autoScroll);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(180.0f);
	ImGui::InputTextWithHint("##search", "検索", gConsole.search, IM_ARRAYSIZE(gConsole.search));

	const float togglesWidth = 220.0f;
	ImGui::SameLine((std::max)(ImGui::GetCursorPosX() + 8.0f, rightEdge - togglesWidth));
	const ImVec4 colLog(0.86f, 0.86f, 0.86f, 1.0f);
	const ImVec4 colWarn(1.0f, 0.80f, 0.30f, 1.0f);
	const ImVec4 colError(1.0f, 0.42f, 0.42f, 1.0f);
	LevelToggle("Log", gConsole.counts[0], &gConsole.showLog, colLog);
	ImGui::SameLine();
	LevelToggle("Warning", gConsole.counts[1], &gConsole.showWarning, colWarn);
	ImGui::SameLine();
	LevelToggle("Error", gConsole.counts[2], &gConsole.showError, colError);
	ImGui::Separator();

	// 表示対象を絞り込む
	const std::string word = gConsole.search;
	std::vector<int> visible;
	visible.reserve(gConsole.lines.size());
	for (int i = 0; i < static_cast<int>(gConsole.lines.size()); ++i) {
		const ConsoleLine& line = gConsole.lines[i];
		const bool levelOn = (line.level == LogLevel::Log && gConsole.showLog) || (line.level == LogLevel::Warning && gConsole.showWarning) ||
		                     (line.level == LogLevel::Error && gConsole.showError);
		if (levelOn && (word.empty() || ContainsIgnoreCase(line.text, word))) {
			visible.push_back(i);
		}
	}

	if (ImGui::BeginChild("##log", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(visible.size()));
		while (clipper.Step()) {
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
				const ConsoleLine& line = gConsole.lines[visible[row]];
				const ImVec4& color = line.level == LogLevel::Error ? colError : line.level == LogLevel::Warning ? colWarn : colLog;
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::TextUnformatted(line.text.c_str());
				ImGui::PopStyleColor();
			}
		}
		// 一番下を見ている時だけ新しいログに追従する
		if (gConsole.autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
			ImGui::SetScrollHereY(1.0f);
		}
	}
	ImGui::EndChild();
	ImGui::End();
#else
	(void)open;
#endif // USE_IMGUI
}

} // namespace TuboEngine::Editor
