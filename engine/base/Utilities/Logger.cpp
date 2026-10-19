#include"Logger.h"
#include<Windows.h>
#include<deque>
#include<mutex>

namespace TuboEngine::Logger {

namespace {
	// ログ履歴（別スレッドから Log されても壊れないよう mutex で守る）
	std::mutex historyMutex;
	std::deque<std::string> history;
	size_t revision = 0;
}

    // log作成
    void Log(const std::string& message) {
		// デバッグ出力
        OutputDebugStringA(message.c_str());

		// Console 表示用に履歴へ残す（末尾の改行は除く）
		std::string line = message;
		while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
			line.pop_back();
		}
		std::lock_guard<std::mutex> lock(historyMutex);
		history.push_back(std::move(line));
		if (history.size() > kMaxHistory) {
			history.pop_front();
		}
		++revision;
    }

	std::vector<std::string> GetHistory() {
		std::lock_guard<std::mutex> lock(historyMutex);
		return std::vector<std::string>(history.begin(), history.end());
	}

	void ClearHistory() {
		std::lock_guard<std::mutex> lock(historyMutex);
		history.clear();
		++revision;
	}

	size_t GetRevision() {
		std::lock_guard<std::mutex> lock(historyMutex);
		return revision;
	}

}