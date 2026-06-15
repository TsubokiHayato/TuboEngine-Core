#include "Audio.h"
#include <algorithm>
#include <iostream>
#undef min

namespace TuboEngine {
Audio::~Audio() {
	// デストラクタで停止してメモリを解放
	Stop();
}

void Audio::Initialize(const std::string& filename) {

	const std::string audioDirectoryPath = "Resources/Audio/";
	// ディレクトリパスのコピー
	directoryPath_ = audioDirectoryPath;

	// WAVファイルを読み込み
	soundDataHandle_ = AudioCommon::GetInstance()->SoundLoadWave(directoryPath_ + filename);
}

void Audio::Play(bool loop, float valume) {
	// 再生
	voiceDataHandle_ = AudioCommon::GetInstance()->SoundPlayWave(soundDataHandle_, loop, valume);
}

void Audio::Stop() {
	// 停止
	if (voiceDataHandle_ != 0u) {
		// ハンドルが有効な場合のみ停止
		AudioCommon::GetInstance()->SoundStop(voiceDataHandle_);
		voiceDataHandle_ = 0u; // ハンドルを無効化
	}
}

void Audio::Pause() {
	// 一時停止
	if (voiceDataHandle_ != 0u) {
		// ハンドルが有効な場合のみ一時停止
		AudioCommon::GetInstance()->SoundPause(voiceDataHandle_);
	}
}

void Audio::Resume() {
	// 再開
	if (voiceDataHandle_ != 0u) {
		// ハンドルが有効な場合のみ再開
		AudioCommon::GetInstance()->SoundResume(voiceDataHandle_);
	}
}

void Audio::SetVolume(float volume) {
	// 音量設定
	if (voiceDataHandle_ != 0u) {
		// ハンドルが有効な場合のみ音量設定
		AudioCommon::GetInstance()->SetVolume(voiceDataHandle_, volume);
	}
}

void Audio::SetPlaybackPosition(float position) {
	playbackPosition_ = position;
	// 再生位置を設定するためのコードを追加
	if (voiceDataHandle_ != 0u) {
		AudioCommon::GetInstance()->SetPlaybackPosition(voiceDataHandle_, (playbackPosition_));
	}
}

void Audio::SetPlaybackSpeed(float speed) {
	playbackSpeed_ = speed;
	// 再生速度を設定するためのコードを追加
	if (voiceDataHandle_ != 0u) {
		AudioCommon::GetInstance()->SetPlaybackSpeed(voiceDataHandle_, playbackSpeed_);
	}
}
//
// void Audio::StartScratch() {
//    isScratching_ = true;
//    scratchThread_ = std::thread(&Audio::ScratchLoop, this);
//}
// void Audio::UpdateScratch(float position) {
//    float duration = GetSoundDuration();
//    scratchPosition_ = std::min(position, duration);
//}
//
// void Audio::StopScratch() {
//    isScratching_ = false;
//    if (scratchThread_.joinable()) {
//        scratchThread_.join();
//    }
//}
//
// void Audio::ScratchLoop() {
//    while (isScratching_) {
//        SetPlaybackPosition(scratchPosition_);
//        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 適切な間隔で更新
//    }
//}
//

float Audio::GetPlaybackPosition() {
	if (soundDataHandle_ != 0u) {
		return AudioCommon::GetInstance()->GetPlaybackPosition(soundDataHandle_);
	}
	return 0.0f;
}

float Audio::GetSoundDuration() const {
	if (soundDataHandle_ != 0u) {
		return AudioCommon::GetInstance()->GetSoundDuration(soundDataHandle_);
	}
	return 0.0f;
}
} // namespace TuboEngine