#include "AudioCommon.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <xaudio2.h>
#include <wrl/client.h>
#pragma comment(lib,"xaudio2.lib")

//サウンドデータコンテナの開始位置
const uint32_t kStartSoundDataIndex = 1;

//サウンドデータのコンテナ
AudioCommon* AudioCommon::GetInstance()
{
	//静的ローカル変数によるシングルトン（new/delete 不要）
	static AudioCommon instance;
	//インスタンスを返す
	return &instance;
}

void AudioCommon::Initialize()
{
	//XAudio2エンジンのインスタンス作成
	HRESULT result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));
	//マスターボイスを作成
	result = xAudio2_->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));

}

void AudioCommon::Finalize()
{
	//コンテナの全開放
	ShutdownContainer();
	//XAudio2の解放
	xAudio2_.Reset();
}

uint32_t AudioCommon::SoundLoadWave(const std::string& filename)
{
	// 1. 既存のサウンドデータを検索
	for (uint32_t i = kStartSoundDataIndex; i < soundDatas_.size(); ++i) {
		if (soundDatas_[i].name == filename) {  // 名前が一致するサウンドデータを発見
			return i;                           // 既存のインデックスを返す
		}
	}

	// 2. ファイルオープン
	std::ifstream file;
	file.open(filename, std::ios_base::binary);
	if (!file.is_open()) {
		std::cerr << "ファイルが開けませんでした: " << filename << std::endl;
		assert(false);
	}

	// 3. 「.wav」データ読み込み

	// ヘッダー情報の読み込み
	RiffHeader riff;
	// ファイルから読み込む
	file.read((char*)&riff, sizeof(riff));
	// チャンクIDの確認
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		// RIFF形式でない場合はエラー
		std::cerr << "RIFF形式ではありません: " << filename << std::endl;
		assert(false);
	}
	// WAVE形式か確認
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		// WAVE形式でない場合はエラー
		std::cerr << "WAVE形式ではありません: " << filename << std::endl;
		assert(false);
	}
	// フォーマットチャンクの読み込み
	FormatChunk format = {};
	// ファイルから読み込む
	file.read((char*)&format, sizeof(ChunkHeader));
	// チャンクIDの確認
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		// fmt形式でない場合はエラー
		std::cerr << "fmt形式ではありません: " << filename << std::endl;
		assert(false);
	}
	//	フォーマットチャンクのサイズが不正な場合はエラー
	if (format.chunk.size > sizeof(format.fmt)) {
		// フォーマットチャンクのサイズが不正な場合はエラー
		std::cerr << "フォーマットチャンクのサイズが不正です: " << filename << std::endl;
		assert(false);
	}
	// フォーマットチャンクの読み込み
	file.read((char*)&format.fmt, format.chunk.size);

	// データチャンクの読み込み
	ChunkHeader data;
	// ファイルから読み込む
	file.read((char*)&data, sizeof(data));
	// チャンクIDの確認
	if (strncmp(data.id, "JUNK", 4) == 0) {
		// JUNK形式の場合はスキップ
		file.seekg(data.size, std::ios_base::cur);
		// 次のチャンクを読み込む
		file.read((char*)&data, sizeof(data));
	}
	// チャンクIDの確認
	if (strncmp(data.id, "data", 4) != 0) {
		// data形式でない場合はエラー
		std::cerr << "data形式ではありません: " << filename << std::endl;
		assert(false);
	}

	// データの読み込み
	char* pBuffer = new char[data.size];
	// ファイルから読み込む
	file.read(pBuffer, data.size);

	// 4. ファイルクローズ
	file.close();

	// 5. サウンドデータの登録
	SoundData soundData = {};
	soundData.wfex = format.fmt;// フォーマット情報をコピー
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);// バッファの先頭アドレスをコピー
	soundData.bufferSize = data.size;// バッファのサイズをコピー
	soundData.name = filename;  // ファイル名を保存

	// soundDatas_に空きがある場所を検索して登録
	for (uint32_t i = kStartSoundDataIndex; i < soundDatas_.size(); ++i) {
		// 空きがある場合は登録
		if (soundDatas_[i].pBuffer == nullptr) { 
			// サウンドデータをコンテナに登録
			soundDatas_[i] = soundData;         
			return i;                           
		}
	}

	// 空きがない場合はエラー
	delete[] pBuffer;
	std::cerr << "No available space in soundDatas_" << std::endl;
	assert(false);
	return -1; 
}

uint32_t AudioCommon::SoundPlayWave(uint32_t soundDataHandle, bool loop, float volume)
{
	

	assert(soundDataHandle < soundDatas_.size()); // ハンドルの範囲チェック
	SoundData& soundData = soundDatas_[soundDataHandle];
	assert(soundData.pBuffer != nullptr); // 有効なサウンドデータが存在することを確認

	// 1. voiceDatas_から既存のボイスデータを検索
	VoiceData* targetVoiceData = nullptr;
	for (VoiceData* voiceData : voiceDatas_) {
		if (voiceData->handle == soundDataHandle) {
			targetVoiceData = voiceData; // 一致するボイスデータを保持
			break;
		}
	}

	// 2. 一致するボイスデータがない場合、新しいボイスデータを作成
	if (targetVoiceData == nullptr) {
		targetVoiceData = new VoiceData();
		targetVoiceData->handle = soundDataHandle;

		HRESULT result = xAudio2_->CreateSourceVoice(&targetVoiceData->sourceVoice, &soundData.wfex);
		assert(SUCCEEDED(result));

		voiceDatas_.insert(targetVoiceData); // 新しいボイスデータをコンテナに登録
	}

	assert(targetVoiceData->sourceVoice != nullptr); // ソースボイスが有効であることを確認

	// 3. 波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	if (loop) {
		buf.LoopCount = XAUDIO2_LOOP_INFINITE; // 無限ループ
	}
	else {
		buf.LoopCount = 0; // ループなし
	}

	// 4. サウンドの再生準備
	HRESULT result = targetVoiceData->sourceVoice->Stop();
	result = targetVoiceData->sourceVoice->FlushSourceBuffers();
	result = targetVoiceData->sourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	// 5. 音量の設定
	result = targetVoiceData->sourceVoice->SetVolume(volume);
	assert(SUCCEEDED(result));

	// 6. サウンドの再生
	result = targetVoiceData->sourceVoice->Start();
	assert(SUCCEEDED(result));

	// 7. 該当するボイスデータの位置を返す
	return soundDataHandle;
}

void AudioCommon::SoundUnload(SoundData* soundData)
{
	//バッファのメモリを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

void AudioCommon::SoundStop(uint32_t voiceHandle)
{
	auto it = std::find_if(voiceDatas_.begin(), voiceDatas_.end(),
		[voiceHandle](VoiceData* data) { return data->handle == voiceHandle; });

	if (it != voiceDatas_.end() && (*it)->sourceVoice) {
		(*it)->sourceVoice->Stop(0);  // サウンドの停止
		(*it)->sourceVoice->FlushSourceBuffers();  // バッファをクリア
	}
}

void AudioCommon::SoundPause(uint32_t voiceHandle)
{
	auto it = std::find_if(voiceDatas_.begin(), voiceDatas_.end(),
		[voiceHandle](VoiceData* data) { return data->handle == voiceHandle; });

	if (it != voiceDatas_.end() && (*it)->sourceVoice) {
		(*it)->sourceVoice->Stop(0);  // 再生を停止（ポーズ）
	}
}

void AudioCommon::SoundResume(uint32_t voiceHandle)
{
	auto it = std::find_if(voiceDatas_.begin(), voiceDatas_.end(),
		[voiceHandle](VoiceData* data) { return data->handle == voiceHandle; });

	if (it != voiceDatas_.end() && (*it)->sourceVoice) {
		(*it)->sourceVoice->Start(0);  // 再生を再開
	}
}

void AudioCommon::SetVolume(uint32_t voiceHandle, float volume)
{
	auto it = std::find_if(voiceDatas_.begin(), voiceDatas_.end(),
		[voiceHandle](VoiceData* data) { return data->handle == voiceHandle; });

	if (it != voiceDatas_.end() && (*it)->sourceVoice) {
		(*it)->sourceVoice->SetVolume(volume);  // 音量の設定
	}
}

void AudioCommon::ClearSoundData() {
	for (auto& soundData : soundDatas_) {
		if (soundData.pBuffer) {
			SoundUnload(&soundData); // バッファを解放
		}
	}
}
void AudioCommon::SetPlaybackPosition(uint32_t voiceDataHandle, float position)
{
	auto it = std::find_if(voiceDatas_.begin(), voiceDatas_.end(),
		[voiceDataHandle](VoiceData* data) { return data->handle == voiceDataHandle; });

	if (it != voiceDatas_.end() && (*it)->sourceVoice) {
		XAUDIO2_VOICE_STATE state;
		(*it)->sourceVoice->GetState(&state);

		// 再生位置を設定
		HRESULT result = (*it)->sourceVoice->Stop();
		assert(SUCCEEDED(result));
		result = (*it)->sourceVoice->FlushSourceBuffers();
		assert(SUCCEEDED(result));

		XAUDIO2_BUFFER buffer = {};
		buffer.pAudioData = soundDatas_[voiceDataHandle].pBuffer;
		buffer.AudioBytes = soundDatas_[voiceDataHandle].bufferSize;
		buffer.Flags = XAUDIO2_END_OF_STREAM;

		// position (秒) をサンプル数に変換
		uint32_t samplePosition = static_cast<uint32_t>(position * soundDatas_[voiceDataHandle].wfex.nSamplesPerSec);
		buffer.PlayBegin = samplePosition;
		buffer.PlayLength = soundDatas_[voiceDataHandle].bufferSize / soundDatas_[voiceDataHandle].wfex.nBlockAlign - samplePosition;

		result = (*it)->sourceVoice->SubmitSourceBuffer(&buffer);
		assert(SUCCEEDED(result));
		result = (*it)->sourceVoice->Start();
		assert(SUCCEEDED(result));
	}
}


void AudioCommon::SetPlaybackSpeed(uint32_t voiceDataHandle, float speed)
{
    auto it = std::find_if(voiceDatas_.begin(), voiceDatas_.end(),
        [voiceDataHandle](VoiceData* data) { return data->handle == voiceDataHandle; });

    if (it != voiceDatas_.end() && (*it)->sourceVoice) {
        (*it)->sourceVoice->SetFrequencyRatio(speed);
    }
}

float AudioCommon::GetPlaybackPosition(uint32_t voiceDataHandle)
{
	auto it = std::find_if(voiceDatas_.begin(), voiceDatas_.end(),
		[voiceDataHandle](VoiceData* data) { return data->handle == voiceDataHandle; });

	if (it != voiceDatas_.end() && (*it)->sourceVoice) {
		XAUDIO2_VOICE_STATE state;
		(*it)->sourceVoice->GetState(&state);
		return static_cast<float>(state.SamplesPlayed) / soundDatas_[voiceDataHandle].wfex.nSamplesPerSec;
	}
	return 0.0f;
}

float AudioCommon::GetSoundDuration(uint32_t soundDataHandle) {
	assert(soundDataHandle < soundDatas_.size());
	const SoundData& soundData = soundDatas_[soundDataHandle];
	return static_cast<float>(soundData.bufferSize) / soundData.wfex.nAvgBytesPerSec;
}




void AudioCommon::ClearVoiceData() {
	for (auto it = voiceDatas_.begin(); it != voiceDatas_.end(); ) {
		VoiceData* voiceData = *it;

		if (voiceData->sourceVoice) {
			voiceData->sourceVoice->Stop();         // 再生を停止
			voiceData->sourceVoice->DestroyVoice(); // ボイスリソースを解放
		}

		delete voiceData; // 動的に確保しているため解放
		it = voiceDatas_.erase(it); // イテレーターを安全に次へ進める
	}
}

void AudioCommon::ShutdownContainer() {
	ClearSoundData(); // SoundDataの解放
	ClearVoiceData(); // VoiceDataの解放
}