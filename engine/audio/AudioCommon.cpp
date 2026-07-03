#include "AudioCommon.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>
#include <cctype>
#include <Windows.h>
#include <xaudio2.h>
#include <wrl/client.h>
// mp3 等のデコード用（Media Foundation）
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#pragma comment(lib,"xaudio2.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")

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

	//Media Foundation の起動（mp3 等のデコードに使用）
	result = MFStartup(MF_VERSION);
	assert(SUCCEEDED(result));
}

void AudioCommon::Finalize()
{
	//コンテナの全開放
	ShutdownContainer();
	//Media Foundation の終了
	MFShutdown();
	//XAudio2の解放
	xAudio2_.Reset();
}

uint32_t AudioCommon::SoundLoad(const std::string& filename)
{
	// 拡張子を小文字化して判定（.wav は従来ローダ、それ以外は Media Foundation）
	std::string ext;
	const size_t dot = filename.find_last_of('.');
	if (dot != std::string::npos) {
		ext = filename.substr(dot + 1);
		for (char& c : ext) {
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
	}

	if (ext == "wav") {
		return SoundLoadWave(filename);
	}
	return SoundLoadMedia(filename);
}

// Media Foundation で任意対応フォーマット（mp3 等）をデコードして PCM として登録する。
uint32_t AudioCommon::SoundLoadMedia(const std::string& filename)
{
	// 1. 既存のサウンドデータを検索（名前一致で使い回す）
	for (uint32_t i = kStartSoundDataIndex; i < soundDatas_.size(); ++i) {
		if (soundDatas_[i].name == filename) {
			return i;
		}
	}

	// 2. UTF-8 パスを UTF-16 へ変換（Media Foundation は wide パス。日本語名も扱える）
	const int wlen = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0);
	if (wlen <= 0) {
		std::cerr << "パス変換に失敗しました: " << filename << std::endl;
		assert(false);
		return static_cast<uint32_t>(-1);
	}
	std::wstring wpath(static_cast<size_t>(wlen), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, wpath.data(), wlen);
	if (!wpath.empty() && wpath.back() == L'\0') {
		wpath.pop_back(); // 末尾の終端 NUL を除去
	}

	// 3. ソースリーダー生成
	Microsoft::WRL::ComPtr<IMFSourceReader> reader;
	HRESULT hr = MFCreateSourceReaderFromURL(wpath.c_str(), nullptr, reader.GetAddressOf());
	if (FAILED(hr)) {
		std::cerr << "音声ファイルを開けませんでした: " << filename << std::endl;
		assert(false);
		return static_cast<uint32_t>(-1);
	}

	const DWORD kAudioStream = static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);

	// 4. 出力を PCM に設定
	Microsoft::WRL::ComPtr<IMFMediaType> pcmType;
	hr = MFCreateMediaType(pcmType.GetAddressOf());
	if (SUCCEEDED(hr)) hr = pcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	if (SUCCEEDED(hr)) hr = pcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	if (SUCCEEDED(hr)) hr = reader->SetCurrentMediaType(kAudioStream, nullptr, pcmType.Get());
	if (SUCCEEDED(hr)) hr = reader->SetStreamSelection(kAudioStream, TRUE);
	if (FAILED(hr)) {
		std::cerr << "PCM 出力の設定に失敗しました: " << filename << std::endl;
		assert(false);
		return static_cast<uint32_t>(-1);
	}

	// 5. 実際の出力フォーマット（WAVEFORMATEX）を取得
	Microsoft::WRL::ComPtr<IMFMediaType> outType;
	hr = reader->GetCurrentMediaType(kAudioStream, outType.GetAddressOf());
	WAVEFORMATEX* pWfx = nullptr;
	UINT32 wfxSize = 0;
	if (SUCCEEDED(hr)) hr = MFCreateWaveFormatExFromMFMediaType(outType.Get(), &pWfx, &wfxSize);
	if (FAILED(hr) || pWfx == nullptr) {
		std::cerr << "フォーマット取得に失敗しました: " << filename << std::endl;
		assert(false);
		return static_cast<uint32_t>(-1);
	}
	WAVEFORMATEX wfex = *pWfx;
	CoTaskMemFree(pWfx);

	// 6. 全サンプルを読み出して PCM を連結
	std::vector<BYTE> pcm;
	for (;;) {
		DWORD flags = 0;
		Microsoft::WRL::ComPtr<IMFSample> sample;
		hr = reader->ReadSample(kAudioStream, 0, nullptr, &flags, nullptr, sample.GetAddressOf());
		if (FAILED(hr)) {
			break;
		}
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
			break;
		}
		if (!sample) {
			continue;
		}
		Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
		if (FAILED(sample->ConvertToContiguousBuffer(buffer.GetAddressOf()))) {
			break;
		}
		BYTE* audioData = nullptr;
		DWORD curLen = 0;
		if (FAILED(buffer->Lock(&audioData, nullptr, &curLen))) {
			break;
		}
		pcm.insert(pcm.end(), audioData, audioData + curLen);
		buffer->Unlock();
	}

	if (pcm.empty()) {
		std::cerr << "デコード結果が空でした: " << filename << std::endl;
		assert(false);
		return static_cast<uint32_t>(-1);
	}

	// 7. サウンドデータの登録（バッファは delete[] で解放されるので new[] で確保）
	BYTE* pBuffer = new BYTE[pcm.size()];
	memcpy(pBuffer, pcm.data(), pcm.size());

	SoundData soundData = {};
	soundData.wfex = wfex;
	soundData.pBuffer = pBuffer;
	soundData.bufferSize = static_cast<unsigned int>(pcm.size());
	soundData.name = filename;

	for (uint32_t i = kStartSoundDataIndex; i < soundDatas_.size(); ++i) {
		if (soundDatas_[i].pBuffer == nullptr) {
			soundDatas_[i] = soundData;
			return i;
		}
	}

	delete[] pBuffer;
	std::cerr << "No available space in soundDatas_" << std::endl;
	assert(false);
	return static_cast<uint32_t>(-1);
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
	// 順序が重要: 先にボイスを停止・破棄してから音声バッファを解放する。
	// 逆にすると、ループ再生中などで生きているボイスを XAudio2 の再生スレッドが
	// 解放済みバッファを読みに行き、終了時にアクセス違反(0xC0000005)を起こす。
	ClearVoiceData(); // VoiceDataの解放（ソースボイスを停止・破棄）
	ClearSoundData(); // SoundDataの解放（バッファを delete[]）
}