#pragma once
#include <string>
#include "AudioCommon.h"
#include <thread>

namespace TuboEngine {
class Audio {
public:
	/// <summary>
	/// デストラクタ
	/// </summary>
	~Audio();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="filename">Resources/はカットして</param>
	/// <param name="directoryPath">ディレクトリパス (デフォルト: "Resources/audios/")</param>
	void Initialize(const std::string& filename);

	/// <summary>
	/// 再生
	/// </summary>
	void Play(bool loop, float valume = 0.5f);

	/// <summary>
	/// 停止
	/// </summary>
	void Stop();

	/// <summary>
	/// 一時停止
	/// </summary>
	void Pause();

	/// <summary>
	/// 再開
	/// </summary>
	void Resume();

	/// <summary>
	/// 音量設定
	/// </summary>
	/// <param name="volume">0.0f〜1.0fの範囲で設定</param>
	void SetVolume(float volume);

	/// <summary>
	/// 再生位置設定
	/// </summary>
	/// <param name="position">再生位置(ミリ秒)</param>
	void SetPlaybackPosition(float position);

	/// <summary>
	/// 再生速度設定
	/// </summary>
	/// <param name="speed">再生速度(1.0fが標準)</param>
	void SetPlaybackSpeed(float speed);

	/// <summary>
	/// 再生位置取得
	/// </summary>
	/// <returns></returns>
	float GetPlaybackPosition();

	/// <summary>
	/// サウンドの長さ取得
	/// </summary>
	/// <returns></returns>
	float GetSoundDuration() const;

	/*---------------
	    スクラッチ
	---------------*/
	///// <summary>
	///// スクラッチ開始
	///// </summary>
	// void StartScratch();

	///// <summary>
	///// スクラッチ更新
	///// </summary>
	///// <param name="position">再生位置</param>
	// void UpdateScratch(float position);

	///// <summary>
	///// スクラッチ停止
	///// </summary>
	// void StopScratch();

private:                            // メンバ変数
	std::string directoryPath_;     // ディレクトリパス
	uint32_t soundDataHandle_ = 0u; // サウンドデータハンドル
	uint32_t voiceDataHandle_ = 0u; // ボイスデータハンドル

	float playbackPosition_ = 0.0f; // 再生位置
	float playbackSpeed_ = 1.0f;    // 再生速度

	//// スクラッチ関連
	// std::thread scratchThread_;
	// std::atomic<bool> isScratching_;
	// std::atomic<float> scratchPosition_;
	///// <summary>
	///// スクラッチループ
	///// </summary>
	////void ScratchLoop();
};
} // namespace TuboEngine