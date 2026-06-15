#include<xaudio2.h>
#include <wrl/client.h>
#include <cstdint>
#ifndef AUDIOCOMMON_H
#define AUDIOCOMMON_H

#include <string>
#include <set>
#include <array>
#include <fstream>
#include <cassert>
#include <vector>
#include <iostream>
#pragma comment(lib,"xaudio2.lib")

class AudioCommon
{
private:

	AudioCommon() = default;//コンストラクタ隠蔽
	~AudioCommon() = default;//デストラクタ隠蔽
	AudioCommon(AudioCommon&) = delete;//コピーコンストラクタ封印
	AudioCommon& operator=(AudioCommon&) = delete;//コピー代入演算子封印
	AudioCommon(AudioCommon&&) = delete;//ムーブコンストラクタ封印
	AudioCommon& operator=(AudioCommon&&) = delete;//ムーブ代入演算子封印

public://シングルトン
	static AudioCommon* GetInstance();
private://非公開構造体
	//チャンクヘッダ
	struct ChunkHeader
	{

		char id[4];//チャンクID
		int32_t size;//チャンクサイズ
	};

	//RIFFヘッダチャンク
	struct RiffHeader
	{
		ChunkHeader chunk;//チャンクヘッダ
		char type[4];//WAVE
	};

	//FMTチャンク
	struct FormatChunk
	{
		ChunkHeader chunk;//チャンクヘッダ
		WAVEFORMATEX fmt;//フォーマット
	};
public://公開構造体
	//音声データ
	struct SoundData
	{

		WAVEFORMATEX wfex;//波形フォーマット

		BYTE* pBuffer;//バッファの先頭アドレス

		unsigned int bufferSize;//バッファのサイズ

		std::string name;//ファイルパス
	};
	// 再生データ
	struct VoiceData {
		
		uint32_t handle = 0u;//アクセスハンドル
		
		IXAudio2SourceVoice* sourceVoice = nullptr;//ソースボイス
	};

public:
	// サウンドデータの最大数
	static const int kMaxSoundData = 128;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// サウンドの読み込み
	/// </summary>
	/// <param name="filename">ファイルの名前</param>
	/// <returns></returns>
	uint32_t SoundLoadWave(const std::string& filename);

	/// <summary>
	/// サウンドの再生
	/// </summary>
	/// <param name="soundDataHandle"></param>
	/// <param name="loop">ループするかどうか</param>
	/// <param name="volume">音の音量</param>
	/// <returns></returns>
	uint32_t SoundPlayWave(uint32_t soundDataHandle, bool loop = false, float volume = 1.0f);

	/// <summary>
	/// サウンドの停止
	/// </summary>
	/// <param name="voiceHandle"></param>
	void SoundStop(uint32_t voiceHandle);

	/// <summary>
	/// サウンドの一時停止
	/// </summary>
	/// <param name="voiceHandle"></param>
	void SoundPause(uint32_t voiceHandle);

	/// <summary>
	/// サウンドの再開
	/// </summary>
	/// <param name="voiceHandle"></param>
	void SoundResume(uint32_t voiceHandle);

	/// <summary>
	/// 音量の設定
	/// </summary>
	/// <param name="voiceHandle"></param>
	/// <param name="volume"></param>
	void SetVolume(uint32_t voiceHandle, float volume);

	/// <summary>
	/// 再生位置の設定
	/// </summary>
	/// <param name="voiceDataHandle"></param>
	/// <param name="position"></param>
	void SetPlaybackPosition(uint32_t voiceDataHandle, float position);

	/// <summary>
	/// 再生速度の設定
	/// </summary>
	/// <param name="voiceDataHandle"></param>
	/// <param name="speed"></param>
	void SetPlaybackSpeed(uint32_t voiceDataHandle, float speed);

	/// <summary>
	/// 再生位置の取得
	/// </summary>
	/// <param name="voiceDataHandle"></param>
	/// <returns></returns>
	float GetPlaybackPosition(uint32_t voiceDataHandle);

	/// <summary>
	/// サウンドデータの解放
	/// </summary>
	/// <param name="soundDataHandle"></param>
	float GetSoundDuration(uint32_t soundDataHandle);


private:
	/// <summary>
	/// サウンドデータの解放
	/// </summary>
	/// <param name="soundData"></param>
	void SoundUnload(SoundData* soundData);

	/// <summary>
	/// ボイスデータの解放
	/// </summary>
	void ClearSoundData();

	/// <summary>
	/// ボイスデータの解放
	/// </summary>
	void ClearVoiceData();

	/// <summary>
	/// コンテナの終了処理
	/// </summary>
	void ShutdownContainer();

private://メンバ変数
	//IXAudio2
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_ = nullptr;
	//マスターボイス
	IXAudio2MasteringVoice* masterVoice;
	//サウンドデータコンテナ
	std::array<SoundData, kMaxSoundData> soundDatas_;
	//ボイスデータコンテナ
	std::set<VoiceData*> voiceDatas_;


};

#endif // !AUDIOCOMMON_H