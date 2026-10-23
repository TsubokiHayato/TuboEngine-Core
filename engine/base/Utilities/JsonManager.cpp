#include "JsonManager.h"
#include "Logger.h"
#include <Windows.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <system_error>

namespace TuboEngine {

namespace {
// exe があるフォルダ
std::filesystem::path ExeDirectory() {
	wchar_t buffer[MAX_PATH] = {};
	const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	if (length == 0 || length >= MAX_PATH) {
		return {};
	}
	return std::filesystem::path(buffer).parent_path();
}
// UTF-8 の文字列 ⇔ パス（C++20 では u8path が非推奨なので char8_t を経由する）
std::filesystem::path ToPath(const std::string& utf8) {
	return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
}
std::string ToUtf8(const std::filesystem::path& path) {
	const std::u8string u8 = path.u8string();
	return std::string(u8.begin(), u8.end());
}
} // namespace

std::string JsonManager::ResolvePath(const std::string& path) {
	const std::filesystem::path p = ToPath(path);
	if (p.is_absolute()) {
		return path;
	}
	std::error_code ec;
	if (std::filesystem::exists(p, ec)) {
		return path; // 作業フォルダにある（VS から起動した時はこちら）
	}
	const std::filesystem::path exeDir = ExeDirectory();
	if (!exeDir.empty()) {
		const std::filesystem::path besideExe = exeDir / p;
		if (std::filesystem::exists(besideExe, ec)) {
			return ToUtf8(besideExe); // exe と同じフォルダにある（配布した exe を直接起動した時）
		}
	}
	return path; // どこにも無い（新しく作る時）は作業フォルダ
}

bool JsonManager::Load(const std::string& path, Json& out, bool logIfMissing) {
	const std::string resolved = ResolvePath(path);
	std::ifstream ifs(ToPath(resolved), std::ios::binary);
	if (!ifs) {
		if (logIfMissing) {
			LogError(path, std::format("ファイルが見つかりません（作業フォルダ: {}）", ToUtf8(std::filesystem::current_path())));
		}
		return false;
	}
	// 空のファイルは「データ無し」として扱う（エラー表示にはしない）
	if (ifs.peek() == std::ifstream::traits_type::eof()) {
		Logger::Log(std::format("[Json] 空のファイルです: {}\n", path));
		return false;
	}
	try {
		out = Json::parse(ifs);
	} catch (const std::exception& e) {
		LogError(path, e.what());
		return false;
	}
	return true;
}

bool JsonManager::Save(const std::string& path, const Json& j, int indent) {
	const std::filesystem::path target = ToPath(ResolvePath(path));
	std::error_code ec;
	if (target.has_parent_path()) {
		std::filesystem::create_directories(target.parent_path(), ec);
		if (ec) {
			LogError(path, std::format("フォルダを作成できません (error={})", ec.value()));
			return false;
		}
	}

	std::string text;
	try {
		text = j.dump(indent);
	} catch (const std::exception& e) {
		LogError(path, e.what()); // 不正な UTF-8 文字列など
		return false;
	}

	// 一時ファイルに書き切ってから置き換える（途中で落ちても元のファイルが残る）
	std::filesystem::path temp = target;
	temp += ".tmp";
	{
		std::ofstream ofs(temp, std::ios::binary | std::ios::trunc);
		if (!ofs) {
			LogError(path, "書き込み用に開けません（読み取り専用・他のアプリが使用中など）");
			return false;
		}
		ofs << text;
		if (!ofs.good()) {
			LogError(path, "書き込みに失敗しました");
			return false;
		}
	}
	if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		std::filesystem::remove(temp, ec);
		LogError(path, std::format("置き換えに失敗しました (GetLastError={})", GetLastError()));
		return false;
	}
	return true;
}

bool JsonManager::Parse(const std::string& text, Json& out) {
	try {
		out = Json::parse(text);
	} catch (const std::exception& e) {
		LogError("(文字列)", e.what());
		return false;
	}
	return true;
}

void JsonManager::LogError(const std::string& path, const std::string& reason) {
	// 「失敗」を含めると Console に赤（Error）で表示される
	Logger::Log(std::format("[Json] 失敗: {} ({})\n", path, reason));
}

} // namespace TuboEngine
