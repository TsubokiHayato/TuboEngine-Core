#pragma once
#include <string>
#include <vector>

namespace TuboEngine {

namespace Logger {
	/// <summary>
	/// ログ出力
	/// </summary>
	/// <param name="message">出力したいメッセージ</param>
    void Log(const std::string& message);

	/// <summary>
	/// これまでのログ履歴（最大 kMaxHistory 件）をコピーして取得する。エディタの Console 表示用。
	/// </summary>
	std::vector<std::string> GetHistory();

	/// <summary>
	/// ログ履歴を消去する。
	/// </summary>
	void ClearHistory();

	/// <summary>
	/// ログが追加/消去されるたびに増える番号（表示側の更新判定用）。
	/// </summary>
	size_t GetRevision();

	// 保持するログの最大件数（古いものから捨てる）
	constexpr size_t kMaxHistory = 2000;
}

} // namespace TuboEngine